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

#include "semantics/blocks/loops.h"

using namespace Semantics;
using namespace Flowgraph;

// We can think of a loop as a combination of a starting value, a condition
// function, and an operation function. The condition function accepts a value
// and returns true if the operation should proceed. The operation function
// accepts the value and applies some mapping, returning a new value. The loop
// passes this output back in as the new starting value and repeats. When the
// condition function returns false, the loop returns the last starting value.

Loop::Loop( Pool &pool, Scope *context ):
	Block(pool, context),
	_condition(pool.Nil()),
	_placeholderIndex(0)
{
}

// Loop::ExitBlock
//
// Generate the condition and operation functions, then make a loop and invoke
// it with the starting-values tuple. The loop will execute the operation
// function, which is our loop body, until the condition is no longer true.
// When iteration ends, the loop function will return its last value; we will
// use this tuple to redefine all affected context variables.
//
Node *Loop::ExitBlock( const SourceLocation &loc )
{
	bool async = IsSegmented();
	bool syncup = SegmentsSynchronize();
	Node *captures = RemapIO( loc );
	Node *condition = ConditionFunction( captures );
	Node *operation = OperationFunction( captures );
	Node *argTuple = StartArgs();
	Node *result = _pool.Nil();
	if (async) {
		Node *loop = syncup ? 
				_pool.Loop_Task( condition, operation, argTuple ):
				_pool.Loop_Sequencer( condition, operation, argTuple );
		Segment::Type::Enum type =
				syncup ? Segment::Type::Sync : Segment::Type::YieldFrom;
		Context().PushSegment( loop, type, BeginLoc() );
		result = _pool.Parameter( 0 );
	} else {
		result = GenerateLoopOperation( argTuple, condition, operation );
	}
	return result;
}

// Loop::StartArgs
//
// Create the initial tuple we will pass in to the loop function. This must
// happen after remapping IO updates. We could do this during the
// RemapIOUpdates function itself, but we want to create an opportunity for
// some hypothetical subclass to do some additional transformation that might
// possibly have some effect on the composition of the arguments tuple. By the
// time Exit gets around to calling this function, the deed is done and
// there will be no further opportunities for refactoring.
//
Node *Loop::StartArgs() const
{
	Node *tuple = _pool.Nil();
	for (NodeMap::const_iterator iter = _updateList.begin();
			iter != _updateList.end(); 
			iter++) {
		Node *startValue = _startValues.find( iter->first )->second;
		tuple = _pool.ArgsAppend( tuple, startValue );
	}
	return _pool.TupleN( tuple );
}

// Loop::RemapIO
//
// At present, the invariants are stored on the IO map along with the updated
// symbols, since we can't know until the end of the loop whether we will
// eventually assign a new value to a symbol. But we have finished the loop, so
// the lists are no longer going to change, and we can now remove invariants
// from the tuple so we don't waste time and memory passing them between
// iterations. We will do this by first making a remap list, then rewriting 
// each output value to use the new values for context vars.
//
Node *Loop::RemapIO( const SourceLocation &loc )
{
	// Before we begin the rewrite process, check our assert chain. If the body
	// of the loop assigned to the assert symbol, that means the loop might
	// have to terminate early as the result of an assertion evaluating false.
	// We must wrap the condition expression in the assert value chain so the
	// loop condition encounters an exception and bails out.
	auto foundUpdatedAssert = _updateList.find( _pool.Sym_Assert() );
	if (foundUpdatedAssert != _updateList.end()) {
		Node *assertChain = foundUpdatedAssert->second;
		_condition = _pool.Chain( assertChain, _condition );
	}

	NodeMap reMap;
	// Iterate through the update list, creating a new parameter list which
	// excludes invariants. We'll put all these mappings into the reMap so we
	// can rewrite them inside various other functions later.
	unsigned int tupleCount = 0;
	for (auto iter: _updateList) {
		// This value is both read and written, which means it needs to go on
		// both our parameter tuple and our output list.
		unsigned int newIndex = tupleCount++;
		Node *oldVal = iter.second;
		assert( oldVal->IsAPlaceholder() );
		Node *paramTuple = _pool.Parameter(0);
		Node *newVal = _pool.Call1( paramTuple, _pool.Number( newIndex ) );
		reMap[oldVal] = newVal;
		ApplyParameterMapping( iter.first, oldVal, newVal );
	}

	// Look for invariants: context vars we read from but never write to. We
	// will remove them from the IO tuple so we don't have to keep passing
	// values between iterations of the loop. Instead we will capture them as
	// slots on the loop function closures.
	unsigned int contextCount = 0;
	Node *captures = _pool.Nil();
	for (NodeMap::const_iterator iter = _invariantList.begin();
			iter != _invariantList.end();
			iter++) {
		Node *sym = iter->first;
		Node *oldVal = iter->second;
		assert( oldVal->IsAPlaceholder() );
		Node *startVal = _startValues.find(sym)->second;
		if (startVal->IsContextIndependent()) {
			// There's no need to capture this, so we'll sub it in for its own
			// placeholder.
			reMap[oldVal] = startVal;
		} else {
			// This is a complex value which must be calculated before we
			// enter the loop. Add it to the list of slots we will capture.
			unsigned int newIndex = contextCount++;
			Node *newVal = _pool.Slot( newIndex );
			reMap[oldVal] = newVal;
			// Append the value for this symbol to the list of expressions to
			// attach to the closure.
			captures = _pool.ArgsAppend( captures, startVal );
		}
	}

	// Commit our rewrites: iterate through the update list, remap each value,
	// then store it. This leaves us a clean set of values which we can return
	// from the operation function. 
	for (NodeMap::const_iterator iter = _updateList.begin();
			iter != _updateList.end();
			iter++) {
		Node *sym = iter->first;
		Node *val = Resolve( sym ).Value();
		val = Flowgraph::Rewrite( val, _pool, reMap );
		Assign( sym, val, loc );
	}
	RewriteSegmentCaptures( reMap );
	assert( _condition );
	_condition = Flowgraph::Rewrite( _condition, _pool, reMap );

	// The return value, now that we have separated the invariants from the
	// updated symbols, is a list of those invariant values which can be
	// attached to the captured loop function closures.
	return captures;
}

// Loop::OperationFunction
//
// Generate the function which actually performs the loop action. This
// basically consists of generating an output tuple, which will either be the
// loop result or the argument value for the next iteration. 
//
Node *Loop::OperationFunction( Node *captures )
{
	// Make up a list of the items we are going to return. Iterate through the
	// context map to populate it.
	Node *result = _pool.Nil();
	for (NodeMap::const_iterator iter = _updateList.begin();
			iter != _updateList.end();
			iter++) {
		result = _pool.ArgsAppend( result, Resolve( iter->first ).Value() );
	}
	result = _pool.TupleN( result );
	if (IsSegmented()) {
		result = PackageSegmentedResult( result );
	}
	// That's it for the loop body function: it accepts all of its context
	// information as part of the parameter tuple. We get a single parameter,
	// which is that tuple.
	std::string name = FullyQualifiedName() + "-operation";
	Node *out = _pool.Function( result, 1, name );
	if (!captures->IsVoid()) {
		out = _pool.CaptureN( out, captures );
	}
	return out;
}

// Loop::ConditionFunction
//
// Generate the function which determines whether we should continue running
// the loop.
//
Node *Loop::ConditionFunction( Node *captures )
{
	std::string name = FullyQualifiedName() + "-condition";
	Node *out = _pool.Function( _condition, 1, name );
	if (!captures->IsVoid()) {
		out = _pool.CaptureN( out, captures );
	}
	return out;
}

// Loop::RebindInContext
//
// Some context var has been updated. Add the symbol to our input/output tuple
// and the list of symbols we have to update on exit.
//
void Loop::RebindInContext(
		Node *name, Node *value, const SourceLocation &loc )
{
	NodeMap::iterator found = _invariantList.find( name );
	if (found != _invariantList.end()) {
		_updateList.insert( std::make_pair( found->first, found->second ) );
		inherited::RebindInContext( name, value, loc );
		_invariantList.erase( found );
	}	
}

// Loop::CreateLocalReference
//
// Go get the current value of this symbol from the context. Allocate a new
// slot on our parameter/result tuple. The working value will be a reference to
// our parameter tuple, at that slot index.
//
Node *Loop::CreateLocalReference( Node *name, Node *value )
{
	// We won't know until we finish the loop whether we are going to refer to
	// this value as a captured slot or through the IO tuple, because we can't
	// know until we're done whether we will in the future assign a new value
	// to this symbol. In the meantime, we'll generate a unique placeholder
	// which we can later remap to the final value.
	Node *ref = _pool.Placeholder( _placeholderIndex++ );
	
	// All captured context variables start out as invariants until we prove
	// they are variable by assigning something to them.
	_invariantList[name] = ref;
	
	// Record whatever value this symbol had in our context before the loop
	// began. We will either use this as an argument to the capture operation
	// or we will pass it in to the loop function as part of the first IO tuple
	// argument, depending on whether it remains invariant.
	_startValues[name] = value;

	// The loop body will refer to the value through its placeholder for now.
	return ref;
}

void Loop::SetConditionExpression( Flowgraph::Node *expression )
{
	assert( _condition && _condition->IsVoid() );
	_condition = expression;
}

Node *Loop::GenerateLoopOperation(
		Node *argTuple, Node *condition, Node *operation )
{
	// By default we just generate a simple loop instruction. This is an
	// opportunity for a subclass to do something sophisticated, should it have
	// clever opinions about other ways to accomplish the same job.
	return _pool.Loop( argTuple, condition, operation );
}


WhileLoop::WhileLoop( Pool &pool, Scope *context ): Loop(pool, context) { }

void WhileLoop::Enter( const AST::While &it )
{
	SetConditionExpression( Eval( it.Exp() ) );
	_beginLoc = it.Location();
}

std::string WhileLoop::FullyQualifiedName() const
{
	return Context().FullyQualifiedName() + ".while-" + _beginLoc.ToString();
}

