// Copyright 2009-2014 Mars Saxman.
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


#include <assert.h>
#include <ctime>
#include <map>
#include <iostream>
#include <stdlib.h>
#include "flowgraph.h"
#include "pool.h"
#include "numtostr.h"

using namespace std;
using namespace Flowgraph;

// Don't use assertions to validate graph construction. Rather, use this CHECK
// CHECK macro, which is basically a deferred assert that only fires if the
// semantics engine never gets around to reporting an error. It's OK to pass
// through invalid states when the input is invalid; it's only a bug if we
// don't also report an error to the user.
#define CHECK(e) \
	if (!(e)) { Taint (#e, __FILE__, __LINE__); return Nil(); }

static uint32_t djb2_hash( const char *str )
{
	uint32_t hash = 5381;
	while (char c = *str++) {
		hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
	}
	return hash;
}

Pool::Cache::~Cache()
{
	for (auto iter = _items.begin(); iter != _items.end(); iter++) {
		assert( iter->second );
		delete iter->second;
	}
}

Node *Pool::Cache::Lookup( std::string key ) const
{
	map<string, Node*>::const_iterator found = _items.find( key );
	return (found != _items.end()) ? found->second : NULL;
}

void Pool::Cache::Store( std::string key, Node *value )
{
	_items[key] = value;
}

template <class T>
Pool::Array<T>::~Array()
{
	for (auto item: _items) {
		delete item;
	}
}

template <class T>
Node *Pool::Array<T>::Lookup( unsigned index )
{
	while (index >= _items.size()) {
		_items.push_back( new T( _items.size() ) );
	}
	return _items[index];
}

Pool::Pool( Delegate &callback, const std::string &filepath ):
	_nil(Value::Type::Void, ""),
	_true(NULL),
	_false(NULL),
	_not(NULL),
	_callback(callback),
	_filePath(filepath),
	_tainted(false)
{
	// Compute a unique prefix ID for private symbols defined in this pool.
	// Each semantic environment gets its own pool, and each module gets its
	// own semantic environment. We will create a unique ID which will only be
	// used by this module; when we create each private symbol, we will add on
	// the prefix so that private symbols are inaccessible to other modules - 
	// referring to the "same" symbol from another module will yield a 
	// different prefix, so the symbols will not actually be equal.
	// We will use a hash of the file path string plus a timestamp; unless we
	// compile the same file path twice within the same second, this will never
	// duplicate, and in that case perhaps we ought to use the same ID anyway.
	_privacyID = numtostr_hex( djb2_hash( filepath.c_str() ) );
	_privacyID += numtostr_hex( std::time( NULL ) );
}

Pool::~Pool()
{
}

// Pool::Validate
//
// We are done generating a program. This pool is finished. Check to see if we
// have generated any invalid flowgraph nodes: if we did, then make sure the
// semantics engine reported an error. We will sometimes create invalid graphs
// in response to invalid input, but we must always report the invalid input to
// the user. If we ever produce an invalid graph without reporting an error,
// that indicates some bug in the compiler.
//
void Pool::Validate( bool didReportError )
{
	if (_tainted && !didReportError) {
		std::cout << _checkMessage << std::endl;
		abort();
	}
}

// Pool::Dummy
//
// The expression parser may generate a "dummy" node when it encounters a
// fatally broken production. The semantic analyzer for expressions will return
// this Dummy value, which is just nil, instead of reporting the dummy node as
// an error. To ensure that some error was reported at some time, however, we
// will mark the pool as tainted. If the parser ever generates a dummy node
// without also reporting an error, the root semantics node will fail an assert
// when compilation finishes.
//
Node *Pool::Dummy()
{
	CHECK( false );
	return Nil();
}

// Pool::Assert
//
// Make sure the condition is true. If so, return it. If not, throw an exception
// using the message value and return that.
//
Node *Pool::Assert( Node *condition, Node *message )
{
	CHECK( condition );
	CHECK( message );
	return Operation( Operation::Type::Assert, condition, message );
}

// Pool::Chain
//
// Chain these two values together. If the left value is an exception, return
// it. Otherwise, return the right value. This is used to bind assertions
// together in a sequence, where the earliest assertion failure wins.
//
Node *Pool::Chain( Node *head, Node *tail )
{
	CHECK( head );
	CHECK( tail );
	if (head == True()) {
		return tail;
	}
	return Operation( Operation::Type::Chain, head, tail );
}

Node *Pool::Function( Node *exp, unsigned int params )
{
	CHECK( exp );
	string key = exp->UniqueID() + " " + numtostr_dec( params );
	Node *out = _functions.Lookup( key );
	if (!out) {
		out = new Flowgraph::Function( exp, params );
		_functions.Store( key, out );
		_callback.PooledFunction( out->AsFunction() );
	}
	return out;
}

Node *Pool::Function( Node *exp, unsigned int params, string name )
{
	CHECK( exp );
	Node *out = _functions.Lookup( name );
	if (!out) {
		out = new Flowgraph::Function( exp, params, name );
		_functions.Store( name, out );
		_callback.PooledFunction( out->AsFunction() );
	}
	else {
		// Functions created by name must be globally unique.
		CHECK( out->AsFunction()->Exp() == exp );
		CHECK( out->AsFunction()->Arity() == params );
	}
	return out;
}

// Pool::Parameter
//
// Every function may accept parameter values.
//
Node *Pool::Parameter( unsigned int index )
{
	return _parameters.Lookup( index );
}

// Pool::Slot
//
// Every function closure has the option to capture context values. The
// function instance, or object, consists of a pointer to the abstract function
// plus a list of context values evaluated at the point of reference. We call
// the storage space for each value a "slot".
//
Node *Pool::Slot( unsigned int index )
{
	return _slots.Lookup( index );
}

// Pool::Placeholder
//
// When generating loops, we need a placeholder to stand in for the value of
// a context symbol which may or may not be part of the loop's IO tuple. We
// won't know until we've finished the loop which it will be, so we have to
// start out with some placeholder value, then rewrite the loop body later.
// Using a dedicated placeholder node type lets us easily verify that the
// rewrite process was complete, since we'll notice quickly if we ever let an
// invalid placeholder out into the next phase of compilation.
//
Node *Pool::Placeholder( unsigned index )
{
	return _placeholders.Lookup( index );
}

Node *Pool::String( string value )
{
	Node *out = _strings.Lookup( value );
	if (!out) {
		out = new Value( Value::Type::String, value );
		_strings.Store( value, out );
	}
	return out;
}

Node *Pool::Number( string value )
{
	Node *out = _numbers.Lookup( value );
	if (!out) {
		out = new Value( Value::Type::Number, value );
		_numbers.Store( value, out );
	}
	return out;
}

Node *Pool::Number( unsigned int value )
{
	return Number( numtostr_dec( value ) );
}

Node *Pool::Float( string value )
{
	Node *out = _floats.Lookup( value );
	if (!out) {
		out = new Value( Value::Type::Float, value );
		_floats.Store( value, out );
	}
	return out;
}


// Pool::Import
//
// Delayed-evaluation placeholder for a module imported from some other source
// file. We will resolve cross-module references later; this lets us split
// compilation across multiple cores. An import reference is based on some
// identifier, which must currently be a module name (symbol).
//
Node *Pool::Import( Node *fileName, Node *sourceDir, const SourceLocation &loc )
{
	CHECK( fileName );
	string key = fileName->UniqueID() + " " + sourceDir->UniqueID();
	Node *out = _imports.Lookup( key );
	if (!out) {
		out = new Flowgraph::Import( fileName, sourceDir );
		_imports.Store( key, out );
		_callback.PooledImport( out->AsImport(), loc );
	}
	return out;
}

// Pool::Symbol
//
// A symbol node is effectively an interned string, the semantic equivalent of
// an identifier token. We use symbol nodes as keys in the symbol table, we use
// them in parameter definitions, and we use them at runtime for method
// dispatch. There are probably other uses too. We guarantee that symbol nodes
// will be unique, so that pointer equality implies string equality. We do not
// impose any particular constraints on the format of a symbol string, but they
// are generally used for identifiers, so we expect that almost all symbol
// strings will follow the identifier syntax. Since Radian identifier matching
// is not case sensitive, we do expect that symbol strings will be provided in
// Unicode's locale-independent case-folding form. This will happen
// automatically for all strings originating from identifier tokens, since the
// scanner performs case-folding as it processes tokens. If you are trying to
// debug problems with case insensitivity, this would be a good place to insert
// some debugging code that checks input strings for case-folding errors.
// 
// This is also the point where we mangle private identifiers. Private idents
// are those which begin with an underscore. We do this so that modules can
// make internal data inaccessible to the outside, which helps separate the
// module's interface from its implementation. We do this on a module-by-module
// basis since we have no classes or other reasonable points of encapsulation.
//
Node *Pool::Symbol( string value )
{
	if (!value.empty() > 0 && value[0] == '_') {
		// The privatized symbol must be otherwise illegal, so that no code
		// will ever accidentally construct it.
		value = _privacyID + ":" + value.substr( 1, std::string::npos );
	}
	Node *out = _symbols.Lookup( value );
	if (!out) {
		out = new Value( Value::Type::Symbol, value );
		_symbols.Store( value, out );
	}
	return out;
}

// Pool::Inductor
//
// An inductor turns some expression into an "induction variable". The only
// purpose of this node is to let us easily track dataflow downstream from the
// induction variable, without having to do a bunch of extra graph traversal.
// The inductor itself is transparent; it will claim to be an operation if you
// ask it. The only difference is that it will always return "true" when you
// ask if it is an induction variable.
//
Node *Pool::Inductor( Node *exp )
{
	CHECK( exp );
	string key = exp->UniqueID();
	Node *out = _inductors.Lookup( key );
	if (!out) {
		out = new Flowgraph::Inductor( exp );
		_inductors.Store( key, out );
	}
	return out;
}

Node *Pool::CallN( Node *object, Node *args )
{
	CHECK( object );
	CHECK( args );
	CHECK( args->IsVoid() || args->IsAnArg() );
	return Operation( Operation::Type::Call, object, args );
}

Node *Pool::Call1( Node *object, Node *arg0 )
{
	return CallN( object, Args1( arg0 ) ); 
}

Node *Pool::Call2( Node *object, Node *arg0, Node *arg1 )
{
	return CallN( object, Args2( arg0, arg1 ) ); 
}

Node *Pool::Call3( Node *object, Node *arg0, Node *arg1, Node *arg2 )
{ 
	return CallN( object, Args3( arg0, arg1, arg2 ) );
}

Node *Pool::Call4( Node *obj, Node *arg0, Node *arg1, Node *arg2, Node *arg3 ) 
{ 
	return CallN( obj, Args4( arg0, arg1, arg2, arg3 ) ); 
}

Node *Pool::CaptureN( Node *function, Node *slots )
{
	CHECK( function );
	CHECK( function->IsAFunction() );
	CHECK( slots );
	CHECK( slots->IsAnArg() );
	return Operation( Operation::Type::Capture, function, slots );
}

Node *Pool::Capture1( Node *object, Node *arg0 )
{
	return CaptureN( object, Args1( arg0 ) );
}

Node *Pool::Capture2( Node *object, Node *arg0, Node *arg1 )
{
	return CaptureN( object, Args2( arg0, arg1 ) );
}

Node *Pool::ArgsAppend( Node *args, Node *val )
{
	CHECK( args );
	CHECK( args->IsAnArg() || args->IsVoid() );
	CHECK( val && !val->IsAnArg() );
	return Operation( Operation::Type::Arg, args, val );
}

Node *Pool::Loop( Node *start, Node *condition, Node *operation )
{
    // The "Loop" operation creates a loop invokable from a condition and an
    // operation. We can then call this invokable, passing in a start value,
    // to iterate over it some number of times and then produce a result.
    // There's no reason the flowgraph could not represent a loop as a normal
    // invokable, but in practice the linearizer pass wants to produce a
    // specific block pattern, and so we will restrict the loop so that it is
    // only called directly and never retained.
    Node *loop = Operation( Operation::Type::Loop, condition, operation );
    return Call1( loop, start );
}

Node *Pool::Loop_Sequencer( Node *condition, Node *operation, Node *value )
{
	return Call3(
			Intrinsic( Intrinsic::ID::Loop_Sequencer ),
			condition,
			operation,
			value );
}

Node *Pool::Loop_Task( Node *condition, Node *operation, Node *value )
{
	return Call3(
			Intrinsic( Intrinsic::ID::Loop_Task ),
			condition,
			operation,
			value );
}

Node *Pool::True()
{
	if (!_true) {
		// True is a function, accepting a true-value and an else-value, which
		// returns the true value.
		_true = Function( Parameter( 0 ), 2, "true" );
	}
	return _true;
}

Node *Pool::False()
{
	if (!_false) {
		// True is a function, accepting a true-value and an else-value, which
		// returns the false value.
		_false = Function( Parameter( 1 ), 2, "false" );
	}
	return _false;
}

Node *Pool::Args2( Node *arg0, Node *arg1 )
{ 
	return ArgsAppend( Args1( arg0 ), arg1 );
}

Node *Pool::Args3( Node *arg0, Node *arg1, Node *arg2 )
{ 
	return ArgsAppend( Args2( arg0, arg1 ), arg2 );
}

Node *Pool::Args4( Node *arg0, Node *arg1, Node *arg2, Node *arg3 )
{ 
	return ArgsAppend( Args3( arg0, arg1, arg2 ), arg3 ); 
}


// Pool::Branch
//
// We take advantage of the fact that booleans are expressed as functions, a la
// Church notation. The boolean function accepts two parameters: the true-value
// and the false-value. The function "True" returns its true-value parameter;
// the function "False" returns its false-value parameter. The condition
// expression will resolve to one of these functions; we will invoke it,
// passing in the "then" and "else" values, and it will return one of them. 
//
Node *Pool::Branch( Node *condition, Node *thenVal, Node *elseVal )
{
	return Call2( condition, thenVal, elseVal );
}

// Pool::Not
//
// Invert the sense of a boolean value.
//
Node *Pool::Not( Node *value )
{
	if (!_not) {
		Node *trueval = Parameter( 0 );
		Node *falseval = Parameter( 1 );
		Node *exp = Slot( 0 );
		Node *result = Call2( exp, falseval, trueval );
		_not = Function( result, 2, "not" );
	}
	return Capture1( _not, value );
}

Node *Pool::TupleAppend( Node *head, Node *tail )
{
	if (head->IsVoid()) {
		return Call1( Intrinsic( Intrinsic::ID::Tuple ), tail );
	}
	else {
		CHECK( head->IsAnOperation() );
		CHECK( head->AsOperation()->Type() == Operation::Type::Call );
		Node *target = head->AsOperation()->Left();
		CHECK( target->IsIntrinsic() );
		CHECK( target->AsIntrinsic()->ID() == Intrinsic::ID::Tuple );
		Node *args = head->AsOperation()->Right();
		return CallN( target, ArgsAppend( args, tail ) );
	}
}

Node *Pool::TupleN( Node *args )
{
	return CallN( Intrinsic( Intrinsic::ID::Tuple ), args );
}

Node *Pool::Tuple1( Node *arg0 )
{ 
	return Call1( Intrinsic( Intrinsic::ID::Tuple ), arg0 );
}

Node *Pool::Tuple2( Node *arg0, Node *arg1 )
{ 
	return Call2( Intrinsic( Intrinsic::ID::Tuple ), arg0, arg1 );
}

Node *Pool::Tuple3( Node *arg0, Node *arg1, Node *arg2 )
{
	return Call3( Intrinsic( Intrinsic::ID::Tuple ), arg0, arg1, arg2 );
}

Node *Pool::Tuple4( Node *arg0, Node *arg1, Node *arg2, Node *arg3 )
{
	return Call4( Intrinsic( Intrinsic::ID::Tuple ), arg0, arg1, arg2, arg3 );
}

// Pool::SetterSymbol
// 
// Create a mangled symbol name for the setter associated with a member
// variable. This does not need to be invokable by mere mortals - in fact it
// probably should not be accessible. We will simply append an equals sign to
// the name.
//
Node *Pool::SetterSymbol( Node *sym )
{
	CHECK( sym && sym->IsASymbol() );
	return SetterSymbol( sym->AsValue()->Contents() );
}

// Pool::Compare
//
// Compare two values by invoking the left operand's compare_to method and
// invoking it, passing in the right operand. This is the "comparable" 
// interface and all of the atoms implement it.
//
Node *Pool::Compare( Node *left, Node *right )
{
	Node *comparator = Call1( left, Sym_Compare_To() );
	return Call2( comparator, left, right );
}

Node *Pool::Throw( Node *exp )
{
	return Call1( Intrinsic( Intrinsic::ID::Throw ), exp );
}

Node *Pool::Catch( Node *exp, Node *handler )
{
	return Call2( Intrinsic( Intrinsic::ID::Catch ), exp, handler ); 
}

Node *Pool::Parallelize( Node *exp )
{ 
	return Call1( Intrinsic( Intrinsic::ID::Parallelize ), exp ); 
}

Node *Pool::IsNotVoid( Node *exp )
{ 
	return Call1( Intrinsic( Intrinsic::ID::IsNotVoid ), exp ); 
}

Node *Pool::IsNotExceptional( Node *exp )
{ 
	return Call1( Intrinsic( Intrinsic::ID::IsNotExceptional ), exp ); 
}

Node *Pool::MapBlank()
{
	return Intrinsic( Intrinsic::ID::Map_Blank ); 
}

Node *Pool::List( Node *exp )
{
	return Call1( Intrinsic( Intrinsic::ID::List ), exp );
}

// Pool::PadLookup
//
// Look for a given node in our scratch pad. This is a way for some module to
// store compound expressions for later use, since there's no point walking
// through the same set of nodes more than once, even if we do end up with the
// same data either way. The pad does not *own* its entries: it is merely a
// bookmark system.
//
Node *Pool::PadLookup( string key ) const
{
	map<string, Node*>::const_iterator found = _pad.find( key );
	return found != _pad.end() ? found->second : NULL;
}

// Pool::PadStore
//
// Lodge this string/expression pair in the scratchpad for later recall.
//
void Pool::PadStore( string key, Node *exp )
{
	assert( _pad.find( key ) == _pad.end() );
	_pad[key] = exp;
}

// Pool::Operation
//
// All non-terminal semantic nodes are operations. All operations accept a left
// and a right node, and represent some combination of those values. This is
// equivalent to a traditional three-address code in SSA form. The pool
// guarantees that each node instance is unique; thus we only generate one
// instance of each (operation, left, right) triplet. We do not offer
// commutative equivalency, because operand order implies evaluation order.
// This is an internal utility common to the public semantic-operation
// accessors. Do not use this to create Guard operations; the Guard method has
// extra logic related to source locations.
//
Node *Pool::Operation( Operation::Type::Enum type, Node *left, Node *right )
{
	CHECK( left );
	CHECK( right );
	// This is a really inefficient way to intern operations, but it works, and
	// that's enough for now. In the future, perhaps we could stuff these
	// values into a struct and use that as the key.  Whatever - we'll figure
	// it out when it becomes a problem.
	string opID = left->UniqueID() + " " + right->UniqueID();
	Node *out = _operations[type].Lookup( opID );
	if (!out) {
		out = new Flowgraph::Operation( type, left, right );
		_operations[type].Store( opID, out );
	}
	return out;
}

// Pool::ImportCore
//
// The compiler delegates the implementation of certain language features to
// the "core" library in the standard library. The interface to this library is
// undocumented; it is private to the compiler. This function returns an import
// reference to it.
//
Node *Pool::ImportCore()
{
    return Import( Sym_Core(), Sym_Radian(), SourceLocation::File(_filePath) );
}

// Pool::Intrinsic
//
// Is this value void?
//
Node *Pool::Intrinsic( Intrinsic::ID::Enum id )
{
	assert( id < Intrinsic::ID::COUNT );
	return _intrinsics.Lookup( id );
}

Node *Pool::BinOp( Intrinsic::ID::Enum id, Node *left, Node *right ) 
{ 
	return Call2( Intrinsic( id ), left, right );
}

// Pool::Taint
//
// Is this condition true? Implementation of a pool-specific assertion scheme.
// If the condition is false, and the pool is not already marked "tainted",
// we will record the message as the check failure message and set the pool's
// taint flag. The concept here is that there are various ways to construct an
// invalid flowgraph which may be the ordinary consequence of a semantic error.
// We definitely don't want to try running such a mangled program, but we don't
// necessarily need to fail an assertion, either. Instead, we'll record the
// fact that the pool is tainted, and at the end of compilation we will assert
// that we have reported some semantic errors. Only if the semantic engine
// failed to report an error will we call this mangled flowgraph a bug in the
// compiler itself.
//
void Pool::Taint( const char *condition, const char *file, int line ) 
{
	if (_tainted) return;
	_tainted = true;
	_checkMessage = string(condition) + ":" + numtostr_dec( line ) + ": " +
			"failed flowgraph check `" + string( condition ) + "'";
}


