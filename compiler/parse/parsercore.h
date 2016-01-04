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

#ifndef parse_parsercore_h
#define parse_parsercore_h

#include "ast/ast.h"
#include "lex/token.h"
#include "utility/sequence.h"
#include "main/error.h"

namespace Parser {

class Core
{
	public:
		Core( Iterator<Token> &input, Reporter &log );
		virtual ~Core() {}

	protected:
		AST::Statement *ParseStatement();
		AST::Expression *ParseExpression();
		AST::Expression *OptionalExpression( Token::Type::Enum flag );
		AST::Expression *OptionalExpression(
				Token::Type::Enum start, Token::Type::Enum end );
		bool NextInputToken();
		bool Next();
		Token Current() const { return _input.Current(); }
		bool IsCurrent( Token::Type::Enum tk );
		bool Match( Token::Type::Enum tk );
		bool Expect( Token::Type::Enum tk );
		bool Expect( Token::Type::Enum tk, Error::Type::Enum err );
		void Synchronize( Token::Type::Enum tk, Error::Type::Enum err );
		void SyntaxError( Error::Type::Enum type );
		SourceLocation Location() const;
		Reporter &_errors;

	private:
		Iterator<Token> &_input;
		SourceLocation _startLoc;
};

} // namespace Parser

#endif //parsercore_h
