// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

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

