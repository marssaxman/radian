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

#include "token.h"
#include <ctype.h>

static bool isident(char c)
{
	return isalnum(c) || '_' == c;
}

static bool isopcode(char c)
{
	switch (c) {
		case '!':
		case '\"':
		case '$':
		case '%':
		case '&':
		case '\'':
		case '*':
		case '+':
		case '-':
		case '.':
		case '/':
		case ':':
		case '<':
		case '=':
		case '>':
		case '?':
		case '@':
		case '^':
		case '`':
		case '|':
		case '~': return true;
		default: return false;
	}
}

token::token(std::string::const_iterator pos, std::string::const_iterator end):
	type(error),
	begin(end),
	end(end)
{
	// skip whitespace
	while (pos < end && isblank(*pos)) {
		++pos;
	}
	begin = pos;
	// if there's at least one char, there's a token; categorize it
	if (pos < end) switch (char c = *pos++) {
		case '\n': type = newline; break;
		case '(': type = lparen; break;
		case ')': type = rparen; break;
		case '[': type = lbracket; break;
		case ']': type = rbracket; break;
		case '{': type = lbrace; break;
		case '}': type = rbrace; break;
		case ',': type = comma; break;
		case ';': type = semicolon; break;
		case '\\':
			type = literal;
			while (pos < end && isalnum(*pos)) ++pos;
			break;
		case '#':
			type = newline; // a comment, actually
			while (pos < end && '\n' != *pos) ++pos;
			break;
		default:
			if (isdigit(c)) {
				type = number;
				while (pos < end && isdigit(*pos)) ++pos;
			} else if (isalpha(c) || '_' == c) {
				type = symbol;
				while (pos < end && isident(*pos)) ++pos;
			} else if (ispunct(c)) {
				type = opcode;
				while (pos < end && isopcode(*pos)) ++pos;
			}
	} else {
		type = eof;
	}
	this->end = pos;
}

