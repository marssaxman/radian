// Copyright 2016 Mars Saxman.
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
// Radian. If not, see <http://www.gnu.org/licenses/>.

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include "lexer.h"
#include "ast.h"

class parser
{
	lexer::iterator source;
	std::ostream &log;
	lexer::token::id current = lexer::token::error;
	bool match(lexer::token::id);
	bool procedure();
	bool terminator();
	bool statement();
	bool assignment();
	bool invocation();
	bool compound();
	bool expression();
	bool infix();
	bool primary();
	bool term();
	bool subscript();
	bool group();
	bool parens();
	bool brackets();
	bool braces();
public:
	parser(lexer::iterator source, std::ostream &log);
};

#endif //PARSER_H

