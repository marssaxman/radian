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
#include "semantics/expressionanalyzer.h"
#include "semantics/closures/function.h"
#include "ast/comprehension.h"

using namespace Semantics;
using namespace Flowgraph;

ExpressionAnalyzer::ExpressionAnalyzer( Pool &pool, Scope &context ) :
	_context(context),
	_pool(pool),
	_lastProcessedSync(NULL)
{
}

// ExpressionAnalyzer::Eval
//
// Main entry point: feed it an expression and it will tell you what that
// expression is worth. Tolerates null expression nodes.
//
Node *ExpressionAnalyzer::Eval( const AST::Expression *it )
{
	Node *out = it ? it->SemGen( this ) : _pool.Nil();
	assert( out );
	return out;
}

// ExpressionAnalyzer::ProcessSyncs
//
// Before diving in to an expression graph, the scope will extract and process
// all sync operations. These have to be handled individually, since each one
// implies a division of execution. Only when all the syncs have been dealt
// with will it actually attempt to evaluate the expression's final result.
//
// Sync processing could also be expressed as a per-statement activity, but we
// trust the statement analyzer to evaluate expressions in left-to-right order.
//
void ExpressionAnalyzer::ProcessSyncs( const AST::Expression *it )
{
	if (!it) return;
	std::queue<const AST::Sync*> list;
	// Perform a depth-first, left-to-right search for sync nodes and create
	// a queue representing the order in which we should evaluate them.
	it->CollectSyncs( &list );
	// Process syncs, each one containing an expression which operates on
	// the result of the previous sync, until we have handled them all.
	while (!list.empty()) {
		_lastProcessedSync = list.front();
		list.pop();
		assert( _lastProcessedSync );
		Node *exp = Eval( _lastProcessedSync->Exp() );
		// Collapse all assertions around this value. We must evaluate the
		// asserts before syncing or we might inadvertently return an invalid
		// value from the sync.
	    Node *assertChain = _context.Resolve( _pool.Sym_Assert() ).Value();
		exp = _pool.Chain( assertChain, exp );
		// Push the expression as a segment delimiter.
		SourceLocation const &loc( _lastProcessedSync->Location() );
		_context.PushSegment( exp, Segment::Type::Sync, loc );
	}
}



Node *ExpressionAnalyzer::GenArgs( const AST::Arguments &it, Node *prefix )
{
	// Unpack the argument list. It's in left-balanced list order, so we need
	// to dig all the way in before we reach the leftmost element.
	std::stack<const AST::Expression *> expstack;
	it.Exp()->UnpackTuple( &expstack );
	// Evaluate the argument value expressions on the stack, then generate an
	// arg node list. Since we're pulling items in left-to-right order, this is
	// the right time to evaluate the expressions.
	Node *args = prefix;
	while (!expstack.empty()) {
		Node *oneArg = Eval( expstack.top() );
		args = _pool.ArgsAppend( args, oneArg );
		expstack.pop();
	}
	return args;
}

Node *ExpressionAnalyzer::SemGen( const AST::Arguments &it )
{
	return GenArgs( it, _pool.Nil() );
}

Node *ExpressionAnalyzer::SemGen( const AST::Identifier &it )
{
	// Reference to an identifier, without a subscript. This may be either a
	// simple variable reference or an invocation of a function which accepts
	// no parameters (aka "thunk").
	Symbol sym = _context.Resolve( _pool.Symbol( it.Value() ) );
	Node *out = _pool.Nil();
	switch (sym.Kind()) {
		case Symbol::Type::Undefined: {
			_context.ReportError( Error::Type::Undefined, it.Location() );
		} break;
		case Symbol::Type::Function: {
			// Direct reference represents evaluation. The value of a function
			// symbol is an invokable; we must generate a call in order to
			// retrieve its value.
			out = _pool.CallN( sym.Value(), _pool.Nil() );
		} break;
		case Symbol::Type::Member: {
			// Direct reference to an object member doesn't work. Must use the
			// "self" reference instead. This symbol type only exists so that
			// we can raise the following error.
			_context.ReportError(
					Error::Type::DirectMemberReference, it.Location() );
		} break;
		default: {
			// If there is no special behavior defined for this symbol, the
			// value we should return is simply whatever was stored in the
			// table.
			out = sym.Value();
		}
	}
	return out;
}

Node *ExpressionAnalyzer::SemGen( const AST::Call &it )
{
	// A call is a reference to a function which accepts parameters. Invoke the
	// function, passing in the argument values.
	assert( it.Exp() && it.Exp()->IsAnIdentifier() );
	const AST::Identifier *ident =
			static_cast<const AST::Identifier*>(it.Exp());
	Symbol sym = _context.Resolve( _pool.Symbol( ident->Value() ) );
	Node *out = _pool.Nil();
	switch (sym.Kind()) {
		case Symbol::Type::Undefined: {
			_context.ReportError( Error::Type::Undefined, ident->Location() );
		} break;
		case Symbol::Type::Function: {
			Node *args = it.Arg()->SemGen( this );
			out = _pool.CallN( sym.Value(), args );
		} break;
		default: {
			_context.ReportError(
					Error::Type::SubscriptNonFunction, ident->Location() );
		} break;
	}
	return out;
}

Node *ExpressionAnalyzer::SemGen( const AST::BooleanLiteral &it )
{
	return it.Value() ? _pool.True() : _pool.False();
}

Node *ExpressionAnalyzer::SemGen( const AST::Dummy &it )
{
	// The parser may return a dummy node when it encounters an invalid
	// expression. It'll have already reported an error.
	return _pool.Dummy();
}

Node *ExpressionAnalyzer::SemGen( const AST::IntegerLiteral &it )
{
	return _pool.Number( it.Value() );
}

Node *ExpressionAnalyzer::SemGen( const AST::RealLiteral &it )
{
	return _pool.Number( it.Value() );
}

Node *ExpressionAnalyzer::SemGen( const AST::FloatLiteral &it )
{
	return _pool.Float( it.Value() );
}

Node *ExpressionAnalyzer::SemGen( const AST::StringLiteral &it )
{
	return _pool.String( it.Value() );
}

Node *ExpressionAnalyzer::SemGen( const AST::SymbolLiteral &it )
{
	return _pool.Symbol( it.Value() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpIf &it )
{
	// The if operator has a condition and two possible results. It is
	// implemented as a compound of the If and Else operators. We expect,
	// therefore, that the right-hand operand will be an Else, which will
	// contain the condition and the else-value. We must only evaluate one of
	// the result expressions; we cannot evaluate both. Therefore we must
	// encapsulate each expression in a nullary function (aka "thunk"). We will
	// use the condition to pick one of these thunks, then evaluate it to get
	// the expression result.
	if (!it.Right()->IsAOpElse()) {
		_context.ReportError(
				Error::Type::IfOperatorWithoutElse, it.Location() );
		return _pool.Nil();
	}

	// Evaluate the condition expression.
	const AST::Expression *conditionExpression =
			static_cast<const AST::OpElse*>(it.Right())->Left();
	Node *expVal = conditionExpression->SemGen( this );

	// Generate the true-expression and false-expression as nullary functions.
	// Generate a branch function which will return one of these functions
	// depending on the value of the condition expression.
	const AST::Expression *thenExpression = it.Left();
	const AST::Expression *elseExpression =
			static_cast<const AST::OpElse*>(it.Right())->Right();
	Node *thenVal = GenerateThunk( thenExpression );
	Node *elseVal = GenerateThunk( elseExpression );

	// Invoke the branch function, passing in the condition expression, to
	// figure out which branch we should evaluate. Invoke the resulting
	// function, with no args, to evaluate our if-expression.
	Node *branchFunc = _pool.Branch( expVal, thenVal, elseVal );
	return _pool.CallN( branchFunc, _pool.Nil() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpElse &it )
{
	// We expect that the else operator will always be used in conjunction with
	// the if. The if-operator code will handle the whole construct, so we
	// would only get into this situation if someone had used an Else without a
	// corresponding If.
	_context.ReportError( Error::Type::ElseOperatorWithoutIf, it.Location() );
	return _pool.Nil();
}

Node *ExpressionAnalyzer::SemGen( const AST::OpTuple &it )
{
	// The comma operator joins its operands into a tuple. An N-length series
	// of commas creates a tuple with order N+1 - it is the number of operands,
	// not the number of commas, that determine the tuple's order. The
	// flowgraph tuple operation creates a new tuple by appending some value to
	// an existing tuple, or to null; we thus create tuples from head to tail,
	// which is to say from left to right. The tuple operator is left-
	// associative, so we expect that our left operand will be either a tuple
	// expression or the initial element. Tuple structure at this stage is
	// lexical, not semantic.
	Node *left = it.Left()->SemGen( this );
	if (!it.Left()->IsAOpTuple()) {
		left = _pool.TupleAppend( _pool.Nil(), left );
	}
	Node *right = it.Right()->SemGen( this );
	// The left operand represents the existing tuple; the right operand
	// represents the value we are going to append to it. The tuple operation
	// takes an existing list and appends a value; this is the opposite of the
	// traditional 'cons' format.
	return _pool.TupleAppend( left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpPair &it )
{
	// The pairing operator, or "hash-arrow" is a dedicated 2-tuple constructor
	// intended to make the map literal syntax more convenient. It could also
	// be used to construct a linked list of order-2 tuples, since it is a
	// right-associative operator.
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	return _pool.Tuple2( left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpEQ &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.False(), _pool.True(), _pool.False() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpNE &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.True(), _pool.False(), _pool.True() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpGE &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.False(), _pool.True(), _pool.True() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpGT &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.False(), _pool.False(), _pool.True() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpLE &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.True(), _pool.True(), _pool.False() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpLT &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *relation = _pool.Compare( left, right );
	return _pool.Call3( relation, _pool.True(), _pool.False(), _pool.False() );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpAdd &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Add() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpSubtract &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Subtract() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpMultiply &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Multiply() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpDivide &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Divide() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpModulus &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Modulus() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpExponent &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Exponentiate() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpShiftLeft &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_ShiftLeft() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpShiftRight &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_ShiftRight() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpAnd &it )
{
	const std::string key = "and";
	Node *func = _pool.PadLookup( key );
	if (!func) {
		Node *trueval = _pool.Parameter( 0 );
		Node *falseval = _pool.Parameter( 1 );
		Node *leftexp = _pool.Slot( 0 );
		Node *rightexp = _pool.Slot( 1 );
		Node *halftrue = _pool.Branch( rightexp, trueval, falseval );
		Node *result = _pool.Branch( leftexp, halftrue, falseval );
		func = _pool.Function( result, 2, key );
		_pool.PadStore( key, func );
	}
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	return _pool.Capture2( func, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpOr &it )
{
	const std::string key = "or";
	Node *func = _pool.PadLookup( key );
	if (!func) {
		Node *trueval = _pool.Parameter( 0 );
		Node *falseval = _pool.Parameter( 1 );
		Node *leftexp = _pool.Slot( 0 );
		Node *rightexp = _pool.Slot( 1 );
		Node *halftrue = _pool.Branch( rightexp, trueval, falseval );
		Node *result = _pool.Branch( leftexp, trueval, halftrue );
		func = _pool.Function( result, 2, key );
		_pool.PadStore( key, func );
	}
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	return _pool.Capture2( func, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpXor &it )
{
	const std::string key = "xor";
	Node *func = _pool.PadLookup( key );
	if (!func) {
		Node *trueval = _pool.Parameter( 0 );
		Node *falseval = _pool.Parameter( 1 );
		Node *leftexp = _pool.Slot( 0 );
		Node *rightexp = _pool.Slot( 1 );
		Node *lefttrue = _pool.Branch( rightexp, falseval, trueval );
		Node *leftfalse = _pool.Branch( rightexp, trueval, falseval );
		Node *result = _pool.Branch( leftexp, lefttrue, leftfalse );
		func = _pool.Function( result, 2, key );
		_pool.PadStore( key, func );
	}
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	return _pool.Capture2( func, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpHas &it )
{
	// A has-expression determines whether some container actually has a value
	// for some key. The container is the left-hand expression; the key is the
	// right-hand expression. We solve this by determining whether the result
	// of the container, called with the key, is an exception or not.
	Node *val = it.Left()->SemGen( this );
	Node *key = it.Right()->SemGen( this );
	Node *result = _pool.Call1( val, key );
	return _pool.IsNotExceptional( result );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpAssert &it )
{
	// An assertion expression associates type information with a given value.
	// The operator wraps its left operand in a guard, taking the right operand
	// as a predicate function which must return true when called with the left
	// operand. This can be used to create type information, since radian types
	// are just functions which filter values to determine whether they match a
	// certain pattern. This is one of a very few higher-order operations built
	// into the language.
	Node *val = it.Left()->SemGen( this );
	Node *filter = it.Right()->SemGen( this );
	Node *condition = _pool.Call1( filter, val );
	Error err(Error::Type::InvalidTypeAssertion, it.Location());
	Node *errtext = _pool.String( err.ToString() );
	Node *throwie = _pool.Intrinsic( Intrinsic::ID::Throw );
	Node *error = _pool.Call1( throwie, errtext );
	return _pool.Branch( condition, val, error );
}

Node *ExpressionAnalyzer::SemGen( const AST::OpConcat &it )
{
	Node *left = it.Left()->SemGen( this );
	Node *right = it.Right()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Concatenate() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::List &it )
{
	// Group a list of values into an ordered, indexed container.
	const AST::Expression *items = it.Items();
	Node *arg = Eval( items );
	if (!items->IsAOpTuple()) {
		arg = _pool.Tuple1( arg );
	}
	return _pool.List( arg );
}

Node *ExpressionAnalyzer::SemGen( const AST::Map &it )
{
	// The map operator lets you create map instances using an array of 
	// key-value assignments. We will start with the map blank, then insert
	// each key-value pair from this tuple.
	Node *map = _pool.MapBlank();
	const AST::Expression *items = it.Items();
	while (items) {
		const AST::Expression *pair = items;
		if (items->IsAOpTuple()) {
			pair = items->AsOpTuple()->Right();
			items = items->AsOpTuple()->Left();
		} else {
			items = NULL;
		}
		if (!pair->IsAOpPair()) {
			_context.ReportError( 
					Error::Type::MapElementsMustBePairs, pair->Location() );
			return map;
		}
		Node *key = Eval( pair->AsOpPair()->Left() );
		Node *value = Eval( pair->AsOpPair()->Right() );
		Node *inserter = _pool.Call1( map, _pool.Sym_Insert() );
		map = _pool.Call3( inserter, map, key, value );
	}
	return map;
}

Node *ExpressionAnalyzer::SemGen( const AST::NegationOp &it )
{
	Node *left = _pool.Number( "0" );
	Node *right = it.Exp()->SemGen( this );
	Node *method = _pool.Call1( left, _pool.Sym_Subtract() );
	return _pool.Call2( method, left, right );
}

Node *ExpressionAnalyzer::SemGen( const AST::NotOp &it )
{
	Node *val = it.Exp()->SemGen( this );
	return _pool.Not( val );
}

Node *ExpressionAnalyzer::SemGen( const AST::SubExpression &it )
{
	return it.Exp()->SemGen( this );
}

Node *ExpressionAnalyzer::SemGen( const AST::Member &it )
{
	// Member reference is syntactic sugar for method invocation. We expect
	// that the left operand is an object, which is the same thing as a method
	// reference, and that the right operand is an identifier with an optional
	// subscript.  We will invoke the object, passing in a symbol for the
	// identifier, expecting to receive a method reference. We will then invoke
	// that method reference, passing in the original object (as the "self"
	// parameter) plus the subscripted arguments if they are present.
	Node *base = Eval( it.Exp() );
	Node *sym = _pool.Nil();
	Node *args = _pool.Args1( base );
	const AST::Expression *member = it.Sub();
	if (member->IsAnIdentifier()) {
		const AST::Identifier *ident =
				static_cast<const AST::Identifier*>(member);
		sym = _pool.Symbol( ident->Value() );
	} else if (member->IsACall()) {
		const AST::Call *call = static_cast<const AST::Call*>(member);
		const AST::Identifier *ident =
				static_cast<const AST::Identifier*>(call->Exp());
		sym = _pool.Symbol( ident->Value() );
		args = GenArgs( *call->Arg(), args );
	} else {
		_context.ReportError(
				Error::Type::MemberMustBeIdentifier, it.Sub()->Location() );
	}
	// Call the object with the symbol to get the method reference.
	Node *method = _pool.Call1( base, sym );
	// Invoke the method, passing in any subscripted argument values.
	return _pool.CallN( method, args );
}

Node *ExpressionAnalyzer::SemGen( const AST::Lookup &it )
{
	Node *base = it.Exp()->SemGen( this );
	Node *key = it.Sub()->SemGen( this );
	Node *method = _pool.Call1( base, _pool.Sym_Lookup() );
	return _pool.Call2( method, base, key );
}

Node *ExpressionAnalyzer::SemGen( const AST::Invoke &it )
{
	// If an expression yields a method reference, and you want to invoke that
	// method, use the invoke operator. This is the same operation implicitly
	// performed by the member-reference operator.
	Node *exp = it.Exp()->SemGen( this );
	Node *args = Eval( it.Sub() );
	return _pool.CallN( exp, args );
}

Node *ExpressionAnalyzer::SemGen( const AST::Lambda &it )
{
	Semantics::Function local(_pool, &_context);
	return local.CaptureLambda( it.Param(), it.Exp() );
}

Node *ExpressionAnalyzer::SemGen( const AST::Throw &it )
{
	// Create an error condition value based on some expression. The error
	// value will "contaminate" any expression it participates in until someone
	// tries to catch it and do something about it.
	Node *exp = it.Exp()->SemGen( this );
	return _pool.Throw( exp );
}

Node *ExpressionAnalyzer::SemGen( const AST::ListComprehension &it )
{
	assert( it.Input() && it.Variable() );
	// Map and/or filter some sequence: for each member in the input sequence,
	// decide whether it matches some predicate expression, then if so map it
	// through some output function and yield its value. This is a lazy process;
	// we don't actually do the mapping and/or filtering right here, but
	// instead we generate a sequence object which has captured all the
	// relevant data and will do the work on each element as it is requested.
	Node *seq = it.Input()->SemGen( this );
	
	// The structure of the list comprehension syntax means we don't know, when
	// we parse the variable clause, whether it is the variable definition or
	// actually the output expression. It depends on what we find later. Thus
	// we need to check now to make sure the variable expression is simply an
	// identifier.
	if (!it.Variable()->IsAnIdentifier()) {
		_context.ReportError(
				Error::Type::DeclarationExpectsIdentifier,
				it.Variable()->Location() );
		return seq;
	}

	Node *core = _pool.ImportCore();

	// If we have a predicate expression, turn it into a function; we'll use it
	// to wrap the sequence in a filter.
	if (it.Predicate()) {
		Semantics::Function local(_pool, &_context);
		Node *predicate = local.CaptureLambda( it.Variable(), it.Predicate() );
		Node *makefilter = _pool.Call1( core, _pool.Sym_Filter() );
		seq = _pool.Call3( makefilter, core, seq, predicate );
	}

	// If we have an output expression, capture it as a function too, mapping
	// sequence values through it.
	if (it.Output()) {
		Semantics::Function local(_pool, &_context);
		Node *output = local.CaptureLambda( it.Variable(), it.Output() );
		Node *makemap = _pool.Call1( core, _pool.Sym_Map() );
		seq = _pool.Call3( makemap, core, seq, output );
	}

	// We started out with a sequence and now we are ending up with a sequence,
	// possibly modified.
	return seq;
}

Node *ExpressionAnalyzer::SemGen( const AST::Sync &it )
{
	// By the time we reach this expression node, we ought to have already
	// handled the sync. This is important because syncs create a division in
	// execution context; we have to handle them before we tackle the overall
	// expression graph. Since each execution context can have only a single
	// sync, we can expect that the last processed sync must be the one we are
	// now evaluating.
	assert( _lastProcessedSync == &it );
	return _pool.Parameter( 0 );
}


// ExpressionAnalyzer::GenerateThunk
//
// Evaluate this expression, but capture it as a nullary function, aka "thunk"
// or lazy future. This is useful when we need to decide whether to actually
// evaluate an expression based on some condition which will not be known until
// run time.
//
Node *ExpressionAnalyzer::GenerateThunk( const AST::Expression *it )
{
	assert( it );
	Semantics::Function local(_pool, &_context);
	return local.CaptureLambda( NULL, it );
}

