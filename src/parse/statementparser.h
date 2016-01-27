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

#ifndef parse_statementparser_h
#define parse_statementparser_h

#include "parse/parsercore.h"

namespace Parser {

class Statement : public Core
{
	public:
		Statement( Iterator<Token> &input, Reporter &log );
		AST::Statement *Result() const { return _currentStatement; }

	protected:
		AST::Statement *ParseStatement();
		AST::Statement *ParseAssertion();
		AST::Statement *ParseAssignment( AST::Expression *target );
		AST::Statement *ParseBlankLine();
		AST::Statement *ParseBlockEnd();
		AST::Statement *ParseDebugTrace();
		AST::Statement *ParseDefinition();
		AST::Statement *ParseElse();
		AST::Statement *ParseExprStatement();
		AST::Statement *ParseForLoop();
		AST::Statement *ParseFunctionDeclaration();
		AST::Statement *ParseIfThen();
		AST::Statement *ParseImport();
		AST::Statement *ParseMethodDeclaration();
		AST::Statement *ParseMutation( AST::Expression *target );
		AST::Statement *ParseObjectDeclaration();
		AST::Statement *ParseSync();
		AST::Statement *ParseVarDeclaration();
		AST::Statement *ParseWhileLoop();
		AST::Statement *ParseYield();
		AST::Expression *ParseTarget();
		AST::Expression *ParseTargetItem();
		Token ExpectDeclIdent();

	private:
		unsigned int _indentLevel;
		AST::Statement *_currentStatement;
};

} // namespace Parser

#endif // statementparser_h
