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

#include "semantics/closures/objects.h"

using namespace Semantics;
using namespace Flowgraph;

Object::Object( Pool &pool, Scope *context ):
	Closure(pool, context),
	_declarationsBecomeMembers(false),
	_members(pool)
{
}

void Object::Enter( const AST::ObjectDeclaration &it )
{
	// Record the object's name, so we can define this symbol later.
	_objectName = it.Name();

	// If the declaration included parameters, create definitions for them now.
	// They will become member vars.
	DefineParameters( it.Parameter(), it.Location() );
	
	// If there is a prototype expression, evaluate it and use the value as the
	// prototype for the memberdispatch object.
	if (it.Prototype()) {
		_members.SetPrototype( Eval( it.Prototype() ) );
	}
	
	// Anything declared inside the object after initial setup will become an
	// object member.
	_declarationsBecomeMembers = true;

	/*
	// debugging thing: add a string that explains where this object came from
	// would be nice to find a permanent way to do this that doesn't suck
	Define(
			_pool.Symbol("debug_origin"),
			_pool.String(it.Location().ToString()),
			Symbol::Type::Def,
			it.Location() );
	*/
}

Scope *Object::Exit( const SourceLocation &loc )
{
	// Create a lookup tree from our list of member definitions. Wrap the
	// result in an object. Define a function in our context which will return
	// this object.
	Node *result = Capture( _members.Result( *this ) );
	Node *sym = _pool.Symbol( _objectName.Value() );
	Context().Define(
			sym, result, Symbol::Type::Function, _objectName.Location() );
	return inherited::Exit( loc );
}

void Object::Define(
	Flowgraph::Node *sym,
	Flowgraph::Node *value,
	Symbol::Type::Enum kind,
	const SourceLocation &loc )
{
	// Once we have entered "memberization mode", anything declared inside an
	// constructor (with the exception of an import) becomes a member of the
	// result object.
	if (_declarationsBecomeMembers && _members.IsMemberizable( kind )) {
		// Add this item to the member index.
		_members.Define( sym, value, kind );
		// We'll create a normal definition with the same name, but we'll use a
		// special symbol type indicating that it is a member, and not to be 
		// used directly - it must be referred to through the "self" variable.
		kind = Symbol::Type::Member;
	}
	inherited::Define( sym, value, kind, loc );
}

void Object::Assign(
		Flowgraph::Node *sym,
		Flowgraph::Node *value,
		const SourceLocation &loc )
{
	// Object members cannot be assigned - only defined.
	// All assignment must occur inside a method body.
	ReportError( Error::Type::ObjectMemberRedefinition, loc );
}

std::string Object::FullyQualifiedName() const
{
	return Context().FullyQualifiedName() + "." + _objectName.Value();
}


Method::Method( Pool &pool, Scope *context ):
	Closure( pool, context )
{
}

void Method::Enter( const AST::MethodDeclaration &it )
{
	_methodName = it.Name();
		
	// Register parameter variables, if any were declared.
	DefineParameters( it.Parameter(), it.Location() );
	
	// Define an assertion chain head. This is traditionally set equal to true
	// since that will be the result of any successful assertion.
	Define( 
			_pool.Sym_Assert(), 
			_pool.True(), 
			Symbol::Type::Var, 
			_methodName.Location() );
}

Scope *Method::Exit( const SourceLocation &loc )
{
	Node *resultValue = Resolve( _pool.Sym_Self() ).Value();
	Node *assertChain = Resolve( _pool.Sym_Assert() ).Value();
	resultValue = _pool.Chain( assertChain, resultValue );
	Node *function = Capture( resultValue );
	
	// Define the method's name in its context, so that following statements
	// can invoke it.
	Node *nameSym = _pool.Symbol( _methodName.Value() );
	Context().Define( 
			nameSym, function, Symbol::Type::Function, _methodName.Location() );
	
	return inherited::Exit( loc );
}

std::string Method::FullyQualifiedName() const
{
	return Context().FullyQualifiedName() + "." + _methodName.Value();
}

