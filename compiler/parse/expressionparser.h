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


#ifndef expressionparser_h
#define expressionparser_h

#include "ast.h"
#include "parsercore.h"

namespace Parser {

class Expression : public Core
{
	public:
		Expression( Iterator<Token> &input, Reporter &log );
		virtual ~Expression() {}
		AST::Expression *Result() const { return _result; }

	protected:
		AST::Arguments *ParseArguments();
		AST::Expression *ParseCapture();
		AST::Expression *ParseEvaluation( const Token &tk );
		AST::Expression *ParseInvoke();
		AST::Expression *ParseList( const Token &tk );
		AST::Expression *ParseListComprehension();
		AST::Expression *ParseMap( const Token &tk );
		AST::Expression *ParsePrimary();
		AST::Expression *ParseRegEx( const Token &tk );
		AST::Expression *ParseSubExpression( const Token &tk );
		AST::Expression *ParseSync();
		AST::Expression *ParseTerm();
		AST::Expression *ParseThrow();
		void SkipOptionalLinebreak();

	private:
		AST::Expression *_result;
};

} // namespace Parser

#endif //expressionparser_h
