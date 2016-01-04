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

#include <stack>
#include "semantics/scope.h"
#include "semantics/closures/function.h"
#include "semantics/blocks/ifthenelse.h"
#include "semantics/blocks/loops.h"
#include "semantics/blocks/foreach.h"
#include "semantics/closures/objects.h"
#include "semantics/statementanalyzer.h"

using namespace Semantics;
using namespace Flowgraph;

StatementAnalyzer::StatementAnalyzer( Pool &pool, Scope *scope ) :
	_pool(pool),
	_scope(scope)
{
	assert( _scope );
}

Scope *StatementAnalyzer::SemGen( const AST::Statement *it )
{
	assert( it );
	it->SemGen( this );
	return _scope;
}

void StatementAnalyzer::GenAssertion( const AST::Assertion &it )
{
	// An assertion is a gateway for the result of the current function. The
	// condition must be true for the result to be valid; otherwise, the
	// program has failed. We handle this with an assertion operation, whose
	// value is equal to its right-hand expression if and only if its left-hand
	// expression evaluates to true. We chain each assertion together, so that
	// the first assertion to fail dominates the following ones.
	// When we are done evaluating the current function, we can combine the
	// collected chain of assertions with the function's result value by
	// chaining the assertion value onto the actual function result.
	Node *chain = _scope->Resolve( _pool.Sym_Assert() ).Value();
	Node *condition = _scope->Eval( it.Condition() );
	Error err(Error::Type::FalseAssertion, it.Location());
	std::string expressionStr = it.Condition()->ToString();
	Node *errtext = _pool.String( err.ToString() + " (" + expressionStr + ")" );
	Node *assert = _pool.Assert( condition, errtext );
	chain = _pool.Chain( chain, assert );
	_scope->Assign( _pool.Sym_Assert(), chain, it.Location() );
}

void StatementAnalyzer::GenDebugTrace( const AST::DebugTrace &it )
{
	// Radian has no debugger, so the only way to debug is to print values to
	// the console or a log file. But printing is an IO action, which means
	// you can only do it from inside another IO action, and that makes life
	// very hard for most of your code!
	// The debug trace statement is a cheaty way around this problem. It lies
	// to the type system and pretends to be an idempotent function, but really
	// it produces output to stderr as a side-effect.
	// The trace function expects three parameters: the first is a string,
	// identifying the source code location where the trace was invoked, the
	// second is the statement's expression, which is presumably some message,
	// and the last is a value which the trace function will return. We can use
	// this to wedge the trace call into a function's evaluation order, by
	// bolting it onto the assert chain - it takes the current assert value as
	// one of its inputs, and the next assert uses the trace function's output,
	// so the trace has to be evaluated (and thus the message printed) as part
	// of the process which determines whether the function succeeded. And
	// that is how we simulate a side-effect producing statement in what is
	// otherwise a pure functional language.
	Node *assertChain = _scope->Resolve( _pool.Sym_Assert() ).Value();
	Node *locStr = _pool.String( it.Location().ToString() );
	Node *exp = _scope->Eval( it.Exp() );
	Node *debugTraceFunc = _pool.Intrinsic( Intrinsic::ID::Debug_Trace);
	assertChain = _pool.Call3( debugTraceFunc, locStr, exp, assertChain );
	_scope->Assign( _pool.Sym_Assert(), assertChain, it.Location() );
}

void StatementAnalyzer::GenAssignment( const AST::Assignment &it )
{
	// An assignment redefines the value of some target, which may be a symbol
	// or a collection of such symbols.
	Node *val = _scope->Eval( it.Right() );
	AssignToTarget( it.Left(), val );
}

void StatementAnalyzer::AssignToTarget(
		const AST::Expression *target, Node *val )
{
	// The specific mechanism of assignment depends on the type of target we
	// are trying to change. This may be a recursive process.
	if (target->IsAnIdentifier()) {
		AssignToIdentifier( target->AsIdentifier(), val );
	}
	else if (target->IsAMember()) {
		AssignToMember( target->AsMember(), val );
	}
	else if (target->IsAOpTuple()) {
		AssignToTuple( target, val );
	}
	else if (target->IsAList()) {
		AssignToList( target->AsList(), val );
	}
	else {
		_scope->ReportError(
				Error::Type::AssignLhsMustBeIdentifier, target->Location() );
	}
}

void StatementAnalyzer::AssignToIdentifier(
		const AST::Identifier *target, Node *val )
{
	Node *ident = _pool.Symbol( target->Value() );
	_scope->Assign( ident, val, target->Location() );
}

void StatementAnalyzer::AssignToMember(
		const AST::Member *target, Node *exp )
{
	std::deque<const AST::Identifier *> names;
	// Our target comes in as a left-leaning chain of identifiers joined as
	// Member nodes. We need to walk this list and unpack the identifiers,
	// validating each one, yielding an ordered list of identifier nodes. If we
	// fail, we'll report an error and then bail out.
	const AST::Expression *crawl = target;
	while (crawl->IsAMember()) {
		const AST::Member *memop = crawl->AsMember();
		const AST::Expression *right = memop->Sub();
		if (!right->IsAnIdentifier()) {
			_scope->ReportError(
					Error::Type::AssignLhsMustBeIdentifier, right->Location() );
			return;
		}
		names.push_front( right->AsIdentifier() );
		crawl = memop->Exp();
	}
	if (!crawl->IsAnIdentifier()) {
		_scope->ReportError(
				Error::Type::AssignLhsMustBeIdentifier, crawl->Location() );
		return;
	}
	names.push_front( crawl->AsIdentifier() );

	// We have a stack of identifiers in left-to-right order which represent
	// the target of this assignment. We also have an expression which is the
	// value to assign. We will start with this value, then work our way back
	// down the identifier stack to compute the intermediate values. When we
	// are down to just one name left, we'll assign whatever we ended up with
	// to that name.
	while (names.size() > 1) {
		// Assign this value to the rightmost element on the stack. We do this
		// by looking up its base expression, then calling a setter method for
		// the rightmost element's name. The value returned by this thing will
		// become the new base value, which we will assign to the ultimate
		// target of this operation.
		const AST::Identifier *target = names.back();
		names.pop_back();
		Node *settername = _pool.SetterSymbol( target->Value() );
		Node *baseobj = _scope->Eval( names.front() );
		// Skip the first item, which is the target variable name; we'll handle
		// that through a conventional assignment when we're done.
		for (auto iter = ++names.begin(); iter != names.end(); iter++) {
			const AST::Identifier *member = *iter;
			Node *membername = _pool.Symbol( member->Value() );
			Node *getter = _pool.Call1( baseobj, membername );
			baseobj = _pool.Call1( getter, baseobj );
		}
		Node *setter = _pool.Call1( baseobj, settername );
		exp = _pool.Call2( setter, baseobj, exp );
	}

	// We've worked our way down the stack of identifiers. All that's left is a
	// single name and a single value. Tell the scope to redefine that name
	// using that value.
	AssignToIdentifier( names.front(), exp );
}

void StatementAnalyzer::AssignToTuple(
		const AST::Expression *target, Node *exp )
{
	assert( target && exp );
	// A syntactic tuple is just a series of comma-separated items. We handle
	// it by treating the expression as a sequence and assigning one element
	// from the sequence to each element in the tuple. This recurses.
	std::stack<const AST::Expression*> expstack;
	target->UnpackTuple( &expstack );
	// We expect the expression to be something that implements the sequence
	// interface. If it is not, we'll get some exception here.
	Node *iterate_func = _pool.Call1( exp, _pool.Sym_Iterate() );
	Node *iterator = _pool.Call1( iterate_func, exp );
	while (!expstack.empty()) {
		target = expstack.top();
		expstack.pop();
		// Assign the iterator's current value to this target.
		Node *element_func = _pool.Call1( iterator, _pool.Sym_Current() );
		Node *element = _pool.Call1( element_func, iterator );
		AssignToTarget( target, element );
		// Get the next iterator, in case we go for another iteration.
		Node *next_func = _pool.Call1( iterator, _pool.Sym_Next() );
		iterator = _pool.Call1( next_func, iterator );
	}
}

void StatementAnalyzer::AssignToList( const AST::List *target, Node *exp )
{
	assert( target && exp );
	// Assignment to a list means we look up values from the expression by 
	// index. The leftmost target gets exp[0], next gets exp[1], and so on.
	// This expects the exp to be a list or a tuple or something like that.
	std::stack<const AST::Expression*> expstack;
	target->Items()->UnpackTuple( &expstack );
	Node *lookup_func = _pool.Call1( exp, _pool.Sym_Lookup() );
	unsigned index = 0;
	while (!expstack.empty()) {
		Node *element = _pool.Call2( lookup_func, exp, _pool.Number( index ) );
		AssignToTarget( expstack.top(), element );
		expstack.pop();
		index++;
	}
}

void StatementAnalyzer::GenMutation( const AST::Mutation &it )
{
	// Mutation is sugar for a specific function call pattern, an elaboration
	// of the member-function pattern. The member-function pattern invokes the
	// base object, passing in the identifier, returning an invokable. The
	// mutator pattern does the same, but the mutator's return value is
	// intended to replace the base object. Thus it is a combination of a
	// function call and an assignment. The tree's form is a target expression
	// and an optional set of arguments.

	const AST::Expression *target = it.Target();
	if (!target->IsAMember()) {
		_scope->ReportError(
				Error::Type::MutatorNeedsMemberIdentifier,
				target->Location() );
		return;
	}
	if (!target->AsMember()->Sub()->IsAnIdentifier()) {
		_scope->ReportError(
				Error::Type::MutatorNeedsMemberIdentifier,
				target->AsMember()->Sub()->Location() );
		return;
	}
	const AST::Identifier *methodIdent =
			target->AsMember()->Sub()->AsIdentifier();
	target = target->AsMember()->Exp();

	// Evaluate the base expression to get an object reference, which we can
	// use to look up the method.
	Node *object = _scope->Eval( target );
	Node *method = _pool.Call1( object, _pool.Symbol( methodIdent->Value() ) );

	// Work out what the arguments for this method should be, including the
	// base object as first parameter.
	Node *args = _pool.Args1( object );
	if (it.Args()) {
		std::stack<const AST::Expression*> exps;
		it.Args()->AsArguments()->Exp()->UnpackTuple( &exps );
		while (!exps.empty()) {
			Node *argval = _scope->Eval( exps.top() );
			args = _pool.ArgsAppend( args, argval );
			exps.pop();
		}
	}

	// Call the method, getting our innermost value, then assign it back to the
	// base expression target.
	Node *exp = _pool.CallN( method, args );
	AssignToTarget( target, exp );
}

void StatementAnalyzer::GenBlankLine( const AST::BlankLine &it )
{
	// Blank lines mean nothing.
}

void StatementAnalyzer::GenBlockEnd( const AST::BlockEnd &it )
{
	// The current block must be finished. We trust the blockstacker to
	// rationalize our input.
	Scope *oldScope = _scope;
	_scope = oldScope->Exit( it.Location() );
	delete oldScope;
}

void StatementAnalyzer::GenVar( const AST::VarDeclaration &it )
{
	Node *sym = _pool.Symbol( it.Name().Value() );
	const AST::Expression *exp = it.Exp();
	Node *value = exp ? _scope->Eval( exp ) : _pool.Undefined();
	_scope->Define( sym, value, Symbol::Type::Var, it.Location() );
}

void StatementAnalyzer::GenDef( const AST::Definition &it )
{
	Node *sym = _pool.Symbol( it.Name().Value() );
	const AST::Expression *exp = it.Exp();
	Node *value = exp ? _scope->Eval( exp ) : _pool.Undefined();
	// we will already have reported a syntax error if there is no exp here
	_scope->Define( sym, value, Symbol::Type::Def, it.Location() );
}

void StatementAnalyzer::GenFunction( const AST::FunctionDeclaration &it )
{
	assert( _scope );
	Function *local = new Function( _pool, _scope );
	local->Enter( it );
	if (it.IsBlockBegin()) {
		_scope = local;
	} else {
		local->Exit( it.Location() );
		delete local;
	}
}

void StatementAnalyzer::GenMethod( const AST::MethodDeclaration &it )
{
	assert( it.IsBlockBegin() );
	Method *local = new Method( _pool, _scope );
	local->Enter( it );
	_scope = local;
}

void StatementAnalyzer::GenImport( const AST::ImportDeclaration &it )
{
	// An import statement pulls a name into the current scope. The current
	// implementation allows only a simple identifier, which is suffixed with
	// ".radian" to create a file name, declaring that identifier as a module.
	// "Name" is the identifier to define and also the base name of the file
	// which should be imported. "Source" is the directory which contains it.
	Node *sourceDir = _pool.Nil();
	if (it.SourceDir()) {
		// Fix this: must support dot-nested directory names
		if (!it.SourceDir()->IsAnIdentifier()) {
			_scope->ReportError(
					Error::Type::ImportSourceMustBeIdentifier,
					it.SourceDir()->Location() );
			return;
		}
		sourceDir = _pool.Symbol( it.SourceDir()->AsIdentifier()->Value() );
	}
	Node *nameStr = _pool.String( it.Name().Value() );
	Node *nameSym = _pool.Symbol( it.Name().Value() );
	Node *val = _pool.Import( nameStr, sourceDir, it.Location() );
	_scope->Define( nameSym, val, Symbol::Type::Import, it.Location() );
}

void StatementAnalyzer::GenObject( const AST::ObjectDeclaration &it )
{
	assert( it.IsBlockBegin() );
	Object *local = new Object( _pool, _scope );
	local->Enter( it );
	_scope = local;
}

void StatementAnalyzer::GenIfThen( const AST::IfThen &it )
{
	assert( it.IsBlockBegin() );
	IfThenElse *local = new IfThenElse( _pool, _scope );
	_scope = local->Enter( it );
}

void StatementAnalyzer::GenElse( const AST::Else &it )
{
	assert( _scope );
	_scope = _scope->PartitionElse( it );
}

void StatementAnalyzer::GenForLoop( const AST::ForLoop &it )
{
	assert( it.IsBlockBegin() );
	ForLoop *local = new ForLoop( _pool, _scope );
	local->Enter( it );
	_scope = local;
}

void StatementAnalyzer::GenSync( const AST::SyncStatement &it )
{
	// The sync statement is a lot like the yield statement, but we use task
	// objects instead of a sequence.
	assert( _scope );
	Node *exp = _scope->Eval( it.Exp() );
	// We must resolve asserts before syncing, because otherwise we might
	// accidentally return an invalid value.
	Node *assertChain = _scope->Resolve( _pool.Sym_Assert() ).Value();
	exp = _pool.Chain( assertChain, exp );
	_scope->PushSegment( exp, Segment::Type::Sync, it.Location() );
}

void StatementAnalyzer::GenWhile( const AST::While &it )
{
	assert( it.IsBlockBegin() );
	WhileLoop *local = new WhileLoop( _pool, _scope );
	local->Enter( it );
	_scope = local;
}

void StatementAnalyzer::GenYield( const AST::Yield &it )
{
	assert( _scope );
	Node *exp = _scope->Eval( it.Exp() );
	Segment::Type::Enum type =
			it.FromSubsequence() ?
			Segment::Type::YieldFrom :
			Segment::Type::Yield;
	_scope->PushSegment( exp, type, it.Location() );
}


