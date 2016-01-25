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

struct token {
	std::string::const_iterator begin;
	std::string::const_iterator end;
	enum {
		error = -1,
		eof = 0,
		number,
		symbol,
		opcode,
		separator,
		opening,
		closing,
		newline
	} type;
	std::string text() const { return std::string(begin, end); }
	token(std::string::const_iterator pos, std::string::const_iterator end);
	bool operator==(const token &other) const;
	bool operator!=(const token &other) const;
};

class lexer
{
	token value;
	std::string::const_iterator enditer;
public:
	lexer(const std::string &input): lexer(input.begin(), input.end()) {}
	lexer(std::string::const_iterator b, std::string::const_iterator e);
	lexer(const lexer &start, const lexer &end);
	operator bool() const { return value.begin != enditer; }
	const token &operator*() const { return value; }
	const token *operator->() const { return &value; }
	lexer &operator++();
	lexer begin() const { return *this; }
	lexer end() const { return lexer(enditer, enditer); }
	bool operator==(const lexer&) const;
	bool operator!=(const lexer&) const;
};

#endif //TOKEN_H

