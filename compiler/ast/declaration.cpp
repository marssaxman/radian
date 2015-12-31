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


#include "declaration.h"

using namespace AST;

Declaration::Declaration(
		Token name, unsigned int indent, const SourceLocation &loc ):
	Statement(indent, loc),
	_name(name)
{
}


VarDeclaration::VarDeclaration(
		Token name,
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc ):
	Declaration( name, indent, loc ),
	_expression(exp)
{
}

VarDeclaration::~VarDeclaration()
{
	if (_expression) {
		delete _expression;
	}
}

Definition::Definition(
		Token name,
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc ):
	Declaration( name, indent, loc ),
	_expression(exp)
{
}

Definition::~Definition()
{
	if (_expression) {
		delete _expression;
	}
}

ParameterizedDeclaration::ParameterizedDeclaration(
		Token name,
		const Expression *param,
		unsigned int indent,
		const SourceLocation &loc):
	Declaration(name, indent, loc),
	_parameter(param)
{
}

ParameterizedDeclaration::~ParameterizedDeclaration()
{
	if (_parameter) delete _parameter;
}


FunctionDeclaration::FunctionDeclaration(
		Token name,
		const Expression *param,
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc):
	ParameterizedDeclaration(name, param, indent, loc),
	_expression(exp)
{
}

FunctionDeclaration::~FunctionDeclaration()
{
	if (_expression) delete _expression;
}


MethodDeclaration::MethodDeclaration(
		Token name,
		const Expression *param,
		unsigned int indent,
		const SourceLocation &loc):
	ParameterizedDeclaration(name, param, indent, loc)
{
}


ImportDeclaration::ImportDeclaration(
		Token name,
		const Expression *sourceDir,
		unsigned int indent,
		const SourceLocation &loc ):
	Declaration( name, indent, loc ),
	_sourceDir( sourceDir )
{
	// source is optional and may be null
}

ImportDeclaration::~ImportDeclaration()
{
	if (_sourceDir) delete _sourceDir;
}


ObjectDeclaration::ObjectDeclaration(
		Token name,
		const Expression *param,
		const Expression *prototype,
		unsigned int indent,
		const SourceLocation &loc ):
	ParameterizedDeclaration(name, param, indent, loc),
	_prototype(prototype)
{
	// prototype is optional and may be null
}

ObjectDeclaration::~ObjectDeclaration()
{
	if (_prototype) delete _prototype;
}

