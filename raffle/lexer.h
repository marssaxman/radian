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

#ifndef LEX_H
#define LEX_H

#include <string>

namespace lexer {

struct token {
	std::string::const_iterator begin;
	std::string::const_iterator end;
	enum id {
		error = -1,
		eof = 0,
		newline = '\n',
		comma = ',',
		semicolon = ';',
		equals = '=',
		ampersand = '&',
		pipe = '|',
		lparen = '(',
		rparen = ')',
		lbracket = '[',
		rbracket = ']',
		lbrace = '{',
		rbrace = '}',
		number,
		symbol,
	} type;
	std::string text() const { return std::string(begin, end); }
	token(std::string::const_iterator pos, std::string::const_iterator end);
	bool operator==(const token &other) const;
	bool operator!=(const token &other) const;
};

class iterator
{
	token value;
	std::string::const_iterator enditer;
public:
	iterator(const std::string &input): iterator(input.begin(), input.end()) {}
	iterator(std::string::const_iterator b, std::string::const_iterator e);
	iterator(const iterator &start, const iterator &end);
	operator bool() const { return value.begin != enditer; }
	const token &operator*() const { return value; }
	const token *operator->() const { return &value; }
	iterator &operator++();
	iterator operator++(int);
	iterator begin() const { return *this; }
	iterator end() const { return iterator(enditer, enditer); }
	bool operator==(const iterator&) const;
	bool operator!=(const iterator&) const;
	bool match(token::id type) const;
};

} // namespace lexer

#endif //LEXER_H

