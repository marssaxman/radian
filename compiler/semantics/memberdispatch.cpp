// Copyright 2010-2012 Mars Saxman.
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

#include "memberdispatch.h"

using namespace Semantics;
using namespace Flowgraph;
using namespace std;


MemberDispatch::MemberDispatch( Pool &pool ):
	_pool(pool),
	_members(_pool.MapBlank()),
	_anyMembersDefined(false)
{
}

// MemberDispatch::SetPrototype
//
// The member map defaults to a blank map, but you can define something else
// instead. This is how inheritance works: you start with some existing members,
// then add new ones (and possibly override old ones). 
//
void MemberDispatch::SetPrototype( Node *prototype )
{
	// This must happen before any members have been defined. After we start
	// defining members, we can no longer "rewind" to the original member map,
	// so there is no way to substitute a different prototype.
	assert( !_anyMembersDefined );
	// We expect the prototype to be an object which was itself created as a
	// memberdispatch output: that is, another closure of the object dispatch
	// function. We expect it to provide us its member map if we call it with
	// the super-sneaky-secret wildcard symbol, which is the same way setter
	// functions work.
	_members = _pool.Call1( prototype, _pool.Sym_Wildcard() );
}

// MemberDispatch::Define
//
// Insert a new entry into our contents map. If it is a function or method,
// it will stand for itself. If it is a const or var, we will wrap it in a
// getter function first, so that everything returned from the map will conform
// to the object protocol. If it is a var, we will additionally generate a
// setter method.
// This scheme assumes that definitions made in a member block will never be
// altered, which is currently true for both objects and modules. If you create
// a new type of member-containing block that allows reassignment, you'll need
// to add an appropriate accessor to MemberDispatch.
//
void MemberDispatch::Define( Node *sym, Node *value, Symbol::Type::Enum kind )
{
	assert( sym && value );
	assert( IsMemberizable( kind ) );
	_anyMembersDefined = true;
	switch (kind) {
		case Symbol::Type::Var: {
			// Create a setter method for this symbol. Insert it using a
			// standard mangled version of the symbol name.
			Node *setter = CustomSetter( sym );
			Node *setsym = _pool.SetterSymbol( sym );
			Node *inserter = _pool.Call1( _members, _pool.Sym_Insert() );
			_members = _pool.Call3( inserter, _members, setsym, setter );
		} // fall through

		case Symbol::Type::Def: {
			// Wrap the value in a getter function so it behaves like a normal
			// object function when the caller invokes it.
			value = WrapGetter( value );
		} // fall through

		case Symbol::Type::Function: {
			// Insert the value into the index.
			Node *inserter = _pool.Call1( _members, _pool.Sym_Insert() );
			_members = _pool.Call3( inserter, _members, sym, value );
		} break;

		case Symbol::Type::Import:
		case Symbol::Type::Member:
		case Symbol::Type::Undefined:
			assert( false );
	}
}

// MemberDispatch::IsMemberizable
//
// Some symbols can be turned into members and others cannot. Which is this?
// It is illegal to tell the memberdispatcher to define a non-memberizable
// symbol, so you should check with this function first.
//
bool MemberDispatch::IsMemberizable( Symbol::Type::Enum kind ) const
{
	switch (kind) {
		case Symbol::Type::Var:
		case Symbol::Type::Def:
		case Symbol::Type::Function:
			return true;
		default:
			return false;
	}
}

// MemberDispatch::Result
//
// Generate the dispatch function, resolving symbols in the specified context.
// The result will be an object: that is, a capture of the object-dispatch
// function, with an index of its members.
//
Node *MemberDispatch::Result( Scope &context ) const
{
	return WrapObject( _members );
}

Node *MemberDispatch::WrapObject( Node *exp ) const
{
	return _pool.Capture1( ObjectFunction(), exp );
}

// MemberDispatch::ObjectFunction
//
// An object is just the closure of a function which will look up a member
// associated with its parameter value. It captures its member list as slot 0.
// The object function also has a special behavior, which is that it will
// return the entire member tree in response to the wildcard selector. This
// allows setter functions to crack the object open, alter the content map, and
// re-capture it as a new object.
//
Node *MemberDispatch::ObjectFunction() const
{
	const string key = "~object";
	Node *out = _pool.PadLookup( key );
	if (!out) {
		Node *selector = _pool.Parameter( 0 );
		Node *members = _pool.Slot( 0 );
		Node *lookup = _pool.Call1( members, _pool.Sym_Lookup() );
		Node *extractor =
				_pool.Function( _pool.Parameter( 0 ), 2, "~object.~extract" );
		Node *relation = _pool.Compare( selector, _pool.Sym_Wildcard() );
		Node *actor = _pool.Call3( relation, lookup, extractor, lookup );
		Node *member = _pool.Call2( actor, members, selector );
		out = _pool.Function( member, 1, key );
		_pool.PadStore( key, out );
	}
	return out;
}


// MemberDispatch::StandardSetter
//
// Utility function which assigns a value to an object member. This is not
// exposed directly; it supports the individual setter methods for specific
// symbols. This method works by cracking the object open and calling assign
// on the member index, passing on the key and value it's been given. Once
// the member index has been modified, it wraps the map back up in an object
// function, which is the output.
//
Node *MemberDispatch::StandardSetter() const
{
	const string key = "~object.~assign";
	Node *out = _pool.PadLookup( key );
	if (!out) {
		Node *self = _pool.Parameter( 0 );
		Node *sym = _pool.Parameter( 1 );
		Node *newval = _pool.Parameter( 2 );
		Node *wrapper = WrapGetter( newval );

		Node *oldtree = _pool.Call1( self, _pool.Sym_Wildcard() );
		Node *setter = _pool.Call1( oldtree, _pool.Sym_Assign() );

		Node *newtree = _pool.Call3( setter, oldtree, sym, wrapper );
		Node *newobj = WrapObject( newtree );

		out = _pool.Function( newobj, 3, key );
		_pool.PadStore( key, out );
	}
	return out;
}

// MemberDispatch::CustomSetter
//
// Wrap the standard setter in a function that sets a specific member. This is
// the function the object will actually return as a specific symbol's setter
// method.
//
Node *MemberDispatch::CustomSetter( Node *sym ) const
{
	Node *self = _pool.Parameter( 0 );
	Node *newval = _pool.Parameter( 1 );
	Node *setter = StandardSetter();
	Node *result = _pool.Call3( setter, self, sym, newval ); 
	string name = "~object.~assign." + sym->AsValue()->Contents();
	return _pool.Function( result, 2, name );
}

Node *MemberDispatch::StandardGetter() const
{
	return _pool.Function( _pool.Slot( 0 ), 1, "~object.~value_wrapper" );
}

Node *MemberDispatch::WrapGetter( Node *exp ) const
{
	return _pool.Capture1( StandardGetter(), exp );
}
