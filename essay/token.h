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

#ifndef TOKEN_H
#define TOKEN_H

#include <string>

struct token
{
	enum {
		error = -1,
		eof = 0,
		number,
		literal,
		symbol,
		opcode,
		comma, // separator
		semicolon, // terminator
		lparen,
		rparen,
		lbracket,
		rbracket,
		lbrace,
		rbrace,
		newline
	} type;
	std::string::const_iterator begin;
	std::string::const_iterator end;
	operator int() const { return type; }
	std::string operator*() const { return std::string(begin, end); }
	token(std::string::const_iterator pos, std::string::const_iterator end);
};

#endif //TOKEN_H

