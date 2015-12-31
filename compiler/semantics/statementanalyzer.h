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


#ifndef statementanalyzer_h
#define statementanalyzer_h

#include "scope.h"
#include "sequence.h"
#include "ast.h"

namespace Semantics {

// The statement analyzer processes an input AST, one statement at a time,
// producing a semantic graph. Each graph component is bound together through
// the symbol table owned by the scope. All statement actions are translated
// into operations on scopes.
class StatementAnalyzer : public AST::StatementAnalyzer
{
	public:
		StatementAnalyzer( Flowgraph::Pool &pool, Scope *scope );
		virtual ~StatementAnalyzer() {}
		Scope *SemGen( const AST::Statement *it );

		// Implementation of AST::StatementAnalyzer
		void GenAssertion( const AST::Assertion &it );
		void GenAssignment( const AST::Assignment &it );
		void GenBlankLine( const AST::BlankLine &it );
		void GenBlockEnd( const AST::BlockEnd &it );
		void GenDebugTrace( const AST::DebugTrace &it );
		void GenDef( const AST::Definition &it );
		void GenElse( const AST::Else &it );
		void GenForLoop( const AST::ForLoop &it );
		void GenFunction( const AST::FunctionDeclaration &it );
		void GenIfThen( const AST::IfThen &it );
		void GenImport( const AST::ImportDeclaration &it );
		void GenMethod( const AST::MethodDeclaration &it );
		void GenMutation( const AST::Mutation &it );
		void GenObject( const AST::ObjectDeclaration &it );
		void GenSync( const AST::SyncStatement &it );
		void GenVar( const AST::VarDeclaration &it );
		void GenWhile( const AST::While &it );
		void GenYield( const AST::Yield &it );

	protected:
		void AssignToTarget(
				const AST::Expression *target, Flowgraph::Node *val );
		void AssignToIdentifier(
				const AST::Identifier *target, Flowgraph::Node *val );
		void AssignToMember(
				const AST::Member *target, Flowgraph::Node *val );
		void AssignToTuple(
				const AST::Expression *target, Flowgraph::Node *val );
		void AssignToList(
				const AST::List *target, Flowgraph::Node *val );

		Flowgraph::Pool &_pool;
		Scope *_scope;
};

} // namespace Semantics

#endif //statementanalyzer_h
