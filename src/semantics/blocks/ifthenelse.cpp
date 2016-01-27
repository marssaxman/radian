// Copyright 2009-2016 Mars Saxman.
//
// Radian is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 2 of the License, or (at your option) any later
// version.
//
// Radian is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// Radian.  If not, see <http://www.gnu.org/licenses/>.

#include "semantics/blocks/ifthenelse.h"

using namespace Semantics;
using namespace Flowgraph;

// We implement conditional evaluation by defining each branch as an anonymous
// function, invoking a boolean to pick one of the two, then calling it. We
// can think of an if-elseif-elseif-else-endif chain as a series of simpler
// if-else-endif blocks. That is, an "else if" effectively begins a new
// if-block inside the else clause of the previous one; both blocks end with
// the same "end if" statement.
//
// Conversely, we must provide a function for both true and false cases, even
// if the user did not provide an else clause. If there is no else, we will
// make up a dummy function which will simply return its input.
//
// The result value is a tuple, providing new values for all of the variables
// the if block might have redefined. That is, the tuple provides new variables
// for all the variables reassigned in the true case *and* all the variables
// reassigned in the false case, regardless of which branch was actually taken.
// If the 'true' branch left a variable alone, but the 'else' branch changed it,
// then the 'true' branch will simply return the variable's original value, and
// vice versa.
//
// This is effectively an implementation of the idea of a 'phi-node', from SSA
// form. (One can think of the flowgraph as a set of assignments to implicit
// variables, where each node defines a separate variable.)

namespace Semantics {

class Branch : public Layer
{
	typedef Layer inherited;
	public:
		Branch(
				Flowgraph::Pool &pool,
				IfThenElse *context,
				Branch *previous,
				const AST::Expression *condition );
		~Branch();
		std::string FullyQualifiedName() const;
		Scope *PartitionElse( const AST::Else &it );
		Scope *Exit( const SourceLocation &loc );
		Node *Result( bool async, Node *previous );
		Branch *Previous() const { return mPrevious; }

	protected:
		Flowgraph::Node *CreateLocalReference(
				Flowgraph::Node *sym, Flowgraph::Node *val );
		virtual void RebindInContext(
				Flowgraph::Node *sym,
				Flowgraph::Node *value,
				const SourceLocation &loc );

	private:
		IfThenElse *_context;
		Branch *mPrevious;
		Flowgraph::Node *_condition;
		unsigned mIndexForFQN;
};

}	// namespace Semantics

IfThenElse::IfThenElse( Pool &pool, Scope *context ):
	Block(pool, context),
	_branchChain(NULL),
	_initialArgs(pool.Nil()),
	_forwardingArgs(pool.Nil()),
	_segmented(false),
	_synchronizes(false)
{
}

Scope *IfThenElse::Enter( const AST::IfThen &it )
{
	_beginLoc = it.Location();
	return new Branch( _pool, this, NULL, it.Exp() );
}

std::string IfThenElse::FullyQualifiedName() const
{
	// as with all nameless blocks, we will use the source location string as
	// the unique ID
	return Context().FullyQualifiedName() + ".if-" + _beginLoc.ToString();
}

Node *IfThenElse::CreateLocalReference( Node *sym, Node *capture )
{
	// Every branch will initially receive all of the context vars as a 
	// parameter. We know this is the first time anyone has asked for this
	// symbol as a context reference, because that's one of the services the
	// Layer class provides for us, so we'll generate a new parameter reference
	// and keep it associated with this symbol. 
	assert( _parameters.find( sym ) == _parameters.end() );
	Node *ref = _pool.Parameter( _parameters.size() );
	_parameters[sym] = ref;
	_initialArgs = _pool.ArgsAppend( _initialArgs, capture );
	_forwardingArgs = _pool.ArgsAppend( _forwardingArgs, ref );
	return ref;
}

Node *IfThenElse::ExitBlock( const SourceLocation &loc )
{
	// Collapse all the branches into a single action function. When we call
	// the action function, it will evaluate the condition expression, then
	// pick one of its branches and execute it. The result will be a tuple
	// corresponding to our array of context rebinds.
	Node *action = _pool.Nil();
	while (_branchChain) {
		action = _branchChain->Result( _segmented, action );
		_branchChain = _branchChain->Previous();
	}
	Node *result = _pool.CallN( action, _initialArgs );
	if (_segmented) {
		Segment::Type::Enum type =
				_synchronizes ? Segment::Type::Sync : Segment::Type::YieldFrom;
		Context().PushSegment( result, type, _beginLoc );
		result = _pool.Parameter( 0 );
	}
	return result;
}


Branch::Branch(
		Pool &pool,
		IfThenElse *context,
		Branch *previous,
		const AST::Expression *condition ):
	Layer(pool, context),
	_context(context),
	mPrevious(previous),
	_condition(pool.Nil()),
	mIndexForFQN(previous ? previous->mIndexForFQN + 1 : 0)
{
	assert( _context );
	_context->_branchChain = this;
	// It is important to evaluate the condition expression from within the
	// branch scope, and not from outside it, because it will actually be
	// evaluated within our output function. That is, our context will invoke
	// some function with some params, and it will produce a tuple; it is up
	// to the function we generate to perform the conditional tests. This is
	// how we can perform lazy evaluation: we only perform the conditional
	// test when we are actually thinking about executing some branch.
	_condition = Eval( condition );
}

Branch::~Branch()
{
	// Each branch is responsible for deleting the previous branch. The very
	// last branch is responsible for deleting the context. We will only get
	// one deletion from the semantic analyzer, so we will use that as our
	// trigger to delete all the other associated scopes.
	if (mPrevious) {
		delete mPrevious;
	} else {
		delete _context;
	}
}

std::string Branch::FullyQualifiedName() const
{
	// we'll use an ascending index to identify each branch of this if-block
	return Context().FullyQualifiedName() + "-" + numtostr_dec( mIndexForFQN );
}

Scope *Branch::PartitionElse( const AST::Else &it )
{
	_context->_segmented |= IsSegmented();
	_context->_synchronizes |= SegmentsSynchronize();
	// We only allow one unconditional else in an if block, and it must come
	// last. It's illegal to partition an unconditional else.
	if (!_condition) {
		ReportError( Error::Type::ElseStatementAfterFinal, it.Location() );
	}
	// Split off a new branch from this one, using the new condition. This new
	// branch will add itself to the context block's branch chain.
	return new Branch( _pool, _context, this, it.Exp() );
}

Scope *Branch::Exit( const SourceLocation &loc )
{
	_context->_segmented |= IsSegmented();
	_context->_synchronizes |= SegmentsSynchronize();
	// When we encounter an end statement, it'll be time to exit the whole
	// if-then block, not just this branch. Since we're just a layer, not a
	// block or closure, we don't have any specific cleanup to do. We'll just
	// tell the context to shut itself down, and it will generate the decision
	// chain and ultimately perform the context assignments.
	// Before we bail out, though, make sure this branch chain ends with an
	// unconditional else. The context if-then block will expect that. If the
	// program did not supply one, we'll synthesize one, which will perform
	// the default action of returning all the original values unchanged.
	if (!_condition->IsVoid()) {
		new Branch( _pool, _context, this, NULL );
	}
	return _context->Exit( loc );
}

Node *Branch::Result( bool async, Node *else_branch )
{
	// Make a function which assembles all of the result values for all of the
	// symbols which have ever been modified anywhere in the if block into a
	// tuple with the canonical order defined by our _contextRebinds.
	// This function will be the action implied by this particular branch.
	Node *tuple = _pool.Nil();
	for (auto sym: _context->_contextRebinds) {
		Node *val = Resolve( sym ).Value();
		tuple = _pool.TupleAppend( tuple, val );
	}
	if (async) {
		// in asynchronous mode, what we return is not the value, but the head
		// of the iterator chain.
		tuple = PackageSegmentedResult( tuple);
	}
	Node *action = _pool.Function( tuple, _context->_parameters.size() );

	// If this is an unconditional branch, our job is done; we'll return this
	// function as it stands. Invoking it with the args array will produce an
	// output tuple which can then be inspected for the new symbol values.
	if (_condition->IsVoid()) {
		assert( else_branch->IsVoid() );
		return action;
	}

	// Combine this action with the else action. We'll evaluate our condition,
	// then use the boolean result to pick between the two branches, then call
	// whichever branch we picked to get the output tuple. The result, again,
	// is a function which, when you call it with the array of input arguments,
	// will produce the desired output tuple. That is, it has the same form as
	// our input else_branch.
	action = _pool.Branch( _condition, action, else_branch );
	action = _pool.CallN( action, _context->_forwardingArgs );
	return _pool.Function( action, _context->_parameters.size() );
}

Flowgraph::Node *Branch::CreateLocalReference( Node *sym, Node *val )
{
	// All branches will share the capture list from the if-then block which
	// contains us. That is, the wrapper is a pass-through, and we will use the
	// values we've captured from it unmodified.
	return _context->_parameters[sym];
}


void Branch::RebindInContext(
		Node *sym, Node *value, const SourceLocation &loc )
{
	Context().Assign( sym, value, loc );
}


