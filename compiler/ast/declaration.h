// Copyright 2009-2013 Mars Saxman.
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


#ifndef declaration_h
#define declaration_h

#include "statement.h"

namespace AST {

class Declaration : public Statement
{
	public:
		Declaration(
				Token name, unsigned int indent, const SourceLocation &loc );
		~Declaration() {}
		Token Name() const { return _name; }
	private:
		Token _name;
};

class VarDeclaration : public Declaration
{
	public:
		VarDeclaration(
				Token name,
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		virtual ~VarDeclaration();
		const AST::Expression *Exp() const { return _expression; }
		void SemGen( StatementAnalyzer *it ) const { it->GenVar( *this ); }
	private:
		const AST::Expression *_expression;
};

class Definition : public Declaration
{
	public:
		Definition(
				Token name,
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~Definition();
		const AST::Expression *Exp() const { return _expression; }
		void SemGen( StatementAnalyzer *it ) const { it->GenDef( *this ); }
	private:
		const AST::Expression *_expression;
};

// Abstract superclass of everything that accepts an optional parameter list.
class ParameterizedDeclaration : public Declaration
{
	public:
		ParameterizedDeclaration(
				Token name,
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~ParameterizedDeclaration();
		const AST::Expression *Parameter() const { return _parameter; }
		Token BlockName() const { return Name(); }
	private:
		const AST::Expression *_parameter;
};

class FunctionDeclaration : public ParameterizedDeclaration
{
	public:
		FunctionDeclaration( Token name,
				const Expression *param,
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~FunctionDeclaration();
		bool IsBlockBegin() const { return !Exp(); }
		const AST::Expression *Exp() const { return _expression; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenFunction( *this ); }
	private:
		const AST::Expression *_expression;
};

class ImportDeclaration : public Declaration
{
	public:
		ImportDeclaration(
				Token name,
				const Expression *sourceDir,
				unsigned int indent,
				const SourceLocation &loc );
		~ImportDeclaration();
		const AST::Expression *SourceDir() const { return _sourceDir; }
		void SemGen( StatementAnalyzer *it ) const { it->GenImport( *this ); }
	private:
		const AST::Expression *_sourceDir;
};

class MethodDeclaration : public ParameterizedDeclaration
{
	public:
		MethodDeclaration(
				Token name,
				const Expression *param,
				unsigned int indent,
				const SourceLocation &loc );
		bool IsBlockBegin() const { return true; }
		void SemGen( StatementAnalyzer *it ) const { it->GenMethod( *this ); }
};

class ObjectDeclaration : public ParameterizedDeclaration
{
	public:
		ObjectDeclaration(
				Token name,
				const Expression *param,
				const Expression *prototype,
				unsigned int indent,
				const SourceLocation &loc );
		~ObjectDeclaration();
		bool IsBlockBegin() const { return true; }
		const AST::Expression *Prototype() const { return _prototype; }
		void SemGen( StatementAnalyzer *it ) const { it->GenObject( *this ); }
	private:
		const AST::Expression *_prototype;
};

} // namespace AST

#endif //declaration_h
