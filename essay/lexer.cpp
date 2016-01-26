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

#include "lexer.h"
#include <ctype.h>

namespace lexer {

using std::string;

static bool isident(char c)
{
	return isalnum(c) || '_' == c;
}

static bool isoperator(char c)
{
	switch (c) {
		case '!':
		case '$':
		case '%':
		case '&':
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
		case '\\':
		case '^':
		case '`':
		case '|':
		case '~': return true;
		default: return false;
	}
}

token::token(string::const_iterator pos, string::const_iterator end):
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
		case '(':
		case '[':
		case '{': type = opening; break;
		case ')':
		case ']':
		case '}': type = closing; break;
		case ',':
		case ';': type = separator; break;
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
			} else if (isoperator(c)) {
				type = opcode;
				while (pos < end && isoperator(*pos)) ++pos;
			}
	} else {
		type = eof;
	}
	this->end = pos;
}

bool token::operator==(const token &other) const
{
	return this->begin == other.begin && this->end == other.end;
}

bool token::operator!=(const token &other) const
{
	return !(*this == other);
}

iterator::iterator(string::const_iterator b, string::const_iterator e):
	value(b, e), enditer(e)
{
}

iterator::iterator(const iterator &start, const iterator &end):
	value(start.value), enditer(end.value.begin)
{
}

iterator &iterator::operator++()
{
	return *this = iterator(value.end, enditer);
}

bool iterator::operator==(const iterator &other) const
{
	return this->value == other.value && this->enditer == other.enditer;
}

bool iterator::operator!=(const iterator &other) const
{
	return !(*this == other);
}

} // namespace lexer


