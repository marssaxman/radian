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

#include <list>
#include <stack>
#include <set>
#include "semantics/blocks/foreach.h"

using namespace Semantics;
using namespace Flowgraph;

ForLoop::ForLoop( Pool &pool, Scope *context ):
	Loop(pool, context),
	_sequence(pool.Nil()),
	_contextIterator(pool.Nil()),
	_iteratorName(pool.Nil()),
	_localIterator(pool.Nil()) {}

// ForLoop::Enter
//
// Evaluate the sequence expression, inside our context, and call its iterate
// function. This gives us the first iterator value, which we will define as a
// context var. Each iteration of the loop will update the value of this var.
// We will then define a local induction variable based on the current value of
// the iterator.
//
void ForLoop::Enter( const AST::ForLoop &it )
{
	_beginLoc = it.Location();

	// We're going to define a context variable which will hold our iterator.
	// Each iteration will update this variable. Having our context own the
	// iterator lets us pass it along from one iteration to the next. There may
	// be more than one loop in the context, so we'll create a unique name for
	// our particular iterator variable.
	std::string locationString = it.Location().ToString();
	_iteratorName = _pool.Symbol( "each-" + locationString );

	// Evaluate the sequence expression and begin iterating over it. This must
	// occur outside the loop, in our context. We will save references to the
	// sequence and the initial iterator in case we need to update them later.
	_sequence = Context().Eval( it.Exp() );
	Node *iteratefunc = _pool.Call1( _sequence, _pool.Sym_Iterate() );
	_contextIterator = _pool.Call1( iteratefunc, _sequence );
	SourceLocation loc = it.Exp()->Location();
	Context().Define( _iteratorName, _contextIterator, Symbol::Type::Var, loc );
	
	// Define an induction variable equal to the current iterator's value. This
	// is the value the loop body wants to work with. Marking the variable
	// with the "inductor" tag lets us track its dependencies later, so we can
	// extract values which depend only on the inductor and on invariants and
	// project them as a map over the input sequence.
	_localIterator = Resolve( _iteratorName ).Value();
	Node *currentfunc = _pool.Call1( _localIterator, _pool.Sym_Current() );
	Node *current = _pool.Call1( currentfunc, _localIterator );
	Node *primeInductor = _pool.Inductor( current );
	Token name = it.BlockName();
	Define( 
			_pool.Symbol( name.Value() ),
			primeInductor,
			Symbol::Type::Var,
			name.Location() );

	// The condition expression is the same for every for loop: we will run the
	// next iteration if our iterator is valid. 
	Node *validfunc = _pool.Call1( _localIterator, _pool.Sym_Is_Valid() );
	SetConditionExpression( _pool.Call1( validfunc, _localIterator ) );
	
	// During the loop body, we'll update the iterator variable to point at the
	// next iterator; this will be part of our output tuple. The next condition
	// function will thus see the next iterator, not the current one, and the
	// next loop body will get the next iterator to start with.
	Node *nextfunc = _pool.Call1( _localIterator, _pool.Sym_Next() );
	Node *nextiter = _pool.Call1( nextfunc, _localIterator );
	Assign( _iteratorName, nextiter, _beginLoc );
}

std::string ForLoop::FullyQualifiedName() const
{
	return Context().FullyQualifiedName() + ".for-" + _beginLoc.ToString();
}

void ForLoop::ApplyParameterMapping(
        Node *key, Node *oldValue, Node *newValue )
{
	assert( key->IsASymbol() );
	if (key == _iteratorName) {
		assert( oldValue == _localIterator );
		_localIterator = newValue;
	}
}


Node *ForLoop::GenerateLoopOperation(
        Node *argTuple, Node *condition, Node *operation )
{
	// This is our chance to do something awesome. We have generated a basic
	// for-loop which contains no async operations. We will analyze the body of
	// the loop, looking for induction variables, which we can extract and
	// apply as a mapping over the original sequence. We will then pass the
	// sequence off to the parallel task dispatcher, which will distribute the
	// work of computing each group of subexpressions across available cores.
	Node *bodyFunc = operation;
	Node *captures = _pool.Nil();
	if (operation->IsACapture()) {
		Operation *lambda = operation->AsOperation();
		bodyFunc = lambda->Left();
		captures = lambda->Right();
	}
	assert( bodyFunc->IsAFunction() );
	assert( captures->IsVoid() || captures->IsAnArg() );
	Node *bodyExp = bodyFunc->AsFunction()->Exp();
	NodeSet mappables;
	FindMappableSubexpressions( bodyExp, &mappables );

	// If we found only a single mappable subexpression, and that subexpression
	// is the prime inductor itself, then the mapping we would create would be
	// a no-op. We record the prime inductor along with other induction vars in
	// case there are references to both, but if we wind up having found only
	// the prime inductor, there's no point in doing any map extraction.
	if (mappables.size() == 1) {
		if ((*mappables.begin())->IsPrimeInductor()) {
			mappables.clear();
		}
	}

	// If we found no mappable subexpressions, it's still worth parallelizing
	// the original loop. The sequence might do some substantial calculation in
	// its "current" function, after all.
	if (mappables.empty()) {
		argTuple = Parallelize( argTuple, _sequence );
		return _pool.Loop( argTuple, condition, operation );
	}

	// Generate a function which computes these subexpressions based on the
	// value from our original input sequence. It should return a tuple in the
	// same order as the list of subexpressions.
	// Call core.map and apply the mapper to our original input sequence.
	Node *mapper = GenerateMapper( mappables, captures );
	Node *core = _pool.ImportCore();
	Node *core_map = _pool.Call1( core, _pool.Sym_Map() );
	Node *newSeq = _pool.Call3( core_map, core, _sequence, mapper );

	// Rewrite the argument tuple so that the loop iterates over the mapped
	// sequence instead of the original sequence.
	argTuple = Parallelize( argTuple, newSeq );

	// Rewrite the operation function so that it expects its values to come in
	// as elements of the tuple our mapper creates. It is not necessary to
	// rewrite the condition function, because it only cares whether the
	// sequence is finished; it doesn't care what kind of sequence it receives.
	NodeMap remap;
	unsigned index = 0;
	Node *currentFunc = _pool.Call1( _localIterator, _pool.Sym_Current() );
	Node *currentVal = _pool.Call1( currentFunc, _localIterator );
	if (mappables.size() > 1) {
		// Rewrite each subexpression as a separate dereference from the tuple
		// returned by the mapping function.
		for (Node *subexp: mappables) {
			Node *indexNumber = _pool.Number( index++ );
			remap[subexp] = _pool.Call1( currentVal, indexNumber );
		}
	} else {
		// There is only one expression, so the mapper will return its value,
		// with no tuple wrapper.
		remap[*(mappables.begin())] = currentVal;
	}
	bodyExp = Flowgraph::Rewrite( bodyExp, _pool, remap );
	unsigned arity = bodyFunc->AsFunction()->Arity();
	std::string name = bodyFunc->AsFunction()->Name();
	// We can't reuse a function name. We'll append an "X" to the name to
	// indicate that this is the parallelized version of the original operation
	// function. This is such a '90s thing to do...
	operation = _pool.Function( bodyExp, arity, name + "X" );
	if (!captures->IsVoid()) {
		operation = _pool.CaptureN( operation, captures );
	}

	// We're done rewriting the components, so now we can build a loop.
    return _pool.Loop( argTuple, condition, operation );
}

void ForLoop::FindMappableSubexpressions( Node *operation, NodeSet *mappables )
{
	// Walk through the expression, looking for induction variables which are
	// complex enough to be worth extracting. Populate the set given to us by
	// the caller. We don't care what was already in the set.
	mappables->clear();
	std::set<Node*> visited;
	std::list<Node*> checkables;
	checkables.push_back(operation);
	for (auto exp: checkables) {
		// Don't look at the same expression more than once, in case the
		// function reuses common subexpressions.
		if (visited.find(exp) != visited.end()) {
			continue;
		}
		visited.insert(exp);
		// Ignore context-independent expressions like constants and imports.
		// By definition, they don't depend on any parameters or slots.
		if (exp->IsContextIndependent()) {
			continue;
		}
		// If the expression is an induction variable, it is a value
		// we should probably extract and compute in the mapping function.
		// If not, perhaps it is a complex expression which we should explore
		// further.
		if (exp->IsInductionVar() && !exp->IsAnArg()) {
			mappables->insert(exp);
		} else if (exp->IsAnOperation()) {
			Operation *op = exp->AsOperation();
			checkables.push_back(op->Left());
			checkables.push_back(op->Right());
		}
	}
}

Node *ForLoop::Parallelize( Node *inputsTuple, Node *inputSequence )
{
	// Parallelize the sequence: that is, wrap it in a dispatcher that will
	// spread the work across multiple processors, if they are available.
	// The interface remains that of an ordinary sequence.
	Node *parallelSequence = _pool.Parallelize( inputSequence );
	Node *parIterFunc = _pool.Call1( parallelSequence, _pool.Sym_Iterate() );
	Node *parallelIterator = _pool.Call1( parIterFunc, parallelSequence );

	// Rebuild the inputs tuple, replacing the original iterator value.
	assert( inputsTuple->IsAnOperation() );
	Node *inputsTupleOp = inputsTuple->AsOperation();
	assert( inputsTupleOp->AsOperation()->Type() == Operation::Type::Call );
	Node *tupleTarget = inputsTupleOp->AsOperation()->Left();
	assert( tupleTarget == _pool.Intrinsic( Intrinsic::ID::Tuple ) );

	// Given the pointer to the arg list passed in to create the tuple, drill
	// in until we find the original context iterator expression. We will
	// replace it with the new iterator we created above. Remember that args
	// are stored in right-to-left order.
	std::stack<Node*> trailingArgVals;
	Node *arg = inputsTupleOp->AsOperation()->Right();
	while (arg->IsAnArg()) {
		Node *previous = arg->AsOperation()->Left();
		Node *value = arg->AsOperation()->Right();
		if (value == _contextIterator) {
			arg = _pool.ArgsAppend( previous, parallelIterator );
			break;
		}
		trailingArgVals.push(value);
		arg = previous;
	}
	// Reconstruct any args which must go to the right of the iterator arg.
	while (!trailingArgVals.empty()) {
		arg = _pool.ArgsAppend( arg, trailingArgVals.top() );
		trailingArgVals.pop();
	}
	// Wrap the whole thing up in a new tuple and return it.
	return _pool.TupleN( arg );
}

Node *ForLoop::GenerateMapper( const NodeSet &mappables, Node *captures )
{
	Node *exp = _pool.Nil();
	if (mappables.size() > 1) {
		// Generate a tuple from the list of mappable subexpressions.
		Node *tupleArgs = _pool.Nil();
		for (auto subexp: mappables) {
			tupleArgs = _pool.ArgsAppend( tupleArgs, subexp );
		}
		exp = _pool.TupleN( tupleArgs );
	} else {
		// Use the output of the solitary subexpression as the map.
		exp = *(mappables.begin());
	}

	// Rewrite the expression so that it gets its value from parameter zero
	// instead of the original expression we were using as the prime inductor,
	// which was based on the local iterator expression.
	NodeMap remap;
	Node *currentFunc = _pool.Call1( _localIterator, _pool.Sym_Current() );
	Node *currentVal = _pool.Call1( currentFunc, _localIterator );
	Node *inductor = _pool.Inductor( currentVal );
	remap[inductor] = _pool.Parameter(0);
	exp = Flowgraph::Rewrite( exp, _pool, remap );

	// Turn this expression into a mapping function suitable for use with
	// core.map.
	std::string blockName = Context().FullyQualifiedName();
	std::string mapperName = blockName + ".mapper-" + _beginLoc.ToString();
	Node *mapper = _pool.Function( exp, 1, mapperName );
	if (!captures->IsVoid()) {
		mapper = _pool.CaptureN( mapper, captures );
	}
	return mapper;
}
