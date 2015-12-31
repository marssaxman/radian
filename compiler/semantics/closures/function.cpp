// Copyright 2009-2012 Mars Saxman.
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


#include "function.h"

using namespace Semantics;

Function::Function( Flowgraph::Pool &pool, Scope *context ):
	Closure( pool, context )
{
}

void Function::Enter( const AST::FunctionDeclaration &it )
{
	_functionName = it.Name();
	
	// Create a reference to self, in case this function (or one of its nested
	// items) wants to recurse.
	Flowgraph::Node *nameSym = _pool.Symbol( _functionName.Value() );
	Define( 
			nameSym, 
			_pool.Self(), 
			Symbol::Type::Function, 
			_functionName.Location() );
	
	EnterFunction( it.Parameter(), it.Exp(), it.Location() );
}

Scope *Function::Exit( const SourceLocation &loc )
{
	Flowgraph::Node *function = ExitFunction();
	
	// Define the function's name in its context, so that following statements
	// can invoke it.
	Flowgraph::Node *nameSym = _pool.Symbol( _functionName.Value() );
	Context().Define( 
			nameSym, 
			function, 
			Symbol::Type::Function, 
			_functionName.Location() );
	
	return inherited::Exit( loc );
}

Flowgraph::Node *Function::CaptureLambda( 
		const AST::Expression *param, const AST::Expression *exp )
{
	assert( exp );
	EnterFunction( param, exp, exp->Location() );
	return ExitFunction();
}

void Function::EnterFunction( 
		const AST::Expression *param, 
		const AST::Expression *exp, 
		const SourceLocation &loc )
{
	// Register parameter variables, if any were declared.
	DefineParameters( param, loc );
	
	// Define an assertion chain head. This may be any non-exceptional value,
	// but it is traditionally equal to true, since that will be the result of
	// any successful assertion check.
	Define( _pool.Sym_Assert(), _pool.True(), Symbol::Type::Var, loc);
   
	// Define a result variable which the function body will reassign. This is
	// the output value for the function call. If the function comes with an
	// expression value, we'll use that as the starting value for the result.
	Flowgraph::Node *val = exp ? Eval( exp ) : _pool.Undefined();
	Define( _pool.Sym_Result(), val, Symbol::Type::Var, loc );
	
	_beginLoc = loc;
}

Flowgraph::Node *Function::ExitFunction()
{
	Flowgraph::Node *resultValue = Resolve( _pool.Sym_Result() ).Value();
	Flowgraph::Node *assertChain = Resolve( _pool.Sym_Assert() ).Value();
	resultValue = _pool.Chain( assertChain, resultValue );
	return Capture( resultValue );
}

std::string Function::FullyQualifiedName() const
{
	// if we have a function name, we'll use it.
	// otherwise, we must be a lambda expression.
	std::string name = _functionName.Value();
	if (!name.size()) name = "lambda-" + _beginLoc.ToString();
	return Context().FullyQualifiedName() + "." + name;
}


