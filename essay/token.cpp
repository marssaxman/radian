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



token::token(const char *pos, const char *end):
	type(error),
	addr(pos),
	len(0)
{
	// skip whitespace
	while (pos < end && isblank(*pos)) {
		type = eof;
		addr = ++pos;
	}
	// if we ran out of input, there's no more work to do
	if (pos == end) {
		return;
	}
	// categorize the token by its first character
	char c = *pos;
	len = 1;
	switch (c) {
		case '\n': type = newline; return;
		case '(': type = lparen; return;
		case ')': type = rparen; return;
		case '[': type = lbracket; return;
		case ']': type = rbracket; return;
		case '{': type = lbrace; return;
		case '}': type = rbrace; return;
		case ',': type = comma; return;
		case ';': type = semicolon; return;
		case '\\':
			type = literal;
			while (++pos < end && isalnum(*pos)) {
				++len;
			}
			return;
		case '#':
			type = newline; // a comment, actually
			while (++pos < end && '\n' != *pos) {
				++len;
			}
			return;
		default: break;
	}
	if (isdigit(c)) {
		type = number;
		while (++pos < end && isdigit(*pos)) {
			++len;
		}
	} else if (isalpha(c) || '_' == c) {
		type = symbol;
		while (++pos < end && (isalnum(*pos) || '_' == *pos)) {
			++len;
		}
	} else if (ispunct(c)) {
		type = opcode;
		while (++pos < end) switch (c = *pos) {
			case '#':
			case '\\':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
			case ',':
			case ';':
			case '_': return;
			default:
				if (!ispunct(c)) return;
				++len;
				continue;
		}
	}
}

token::token(const std::string &src):
	token(src.c_str(), src.c_str() + src.size())
{
}

