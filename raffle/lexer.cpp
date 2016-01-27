// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#include "lexer.h"
#include <ctype.h>

namespace lexer {

using std::string;

static bool isidentstart(char c)
{
	return isalpha(c) || '_' == c;
}

static bool isidentbody(char c)
{
	return isalnum(c) || '_' == c;
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
		case '\n':
		case '(':
		case '[':
		case '{':
		case ')':
		case ']':
		case '}':
		case ',':
		case ';': type = static_cast<enum id>(c); break;
		case '#':
			// comments are reported as newline characters, but we must
			// skip over all the comment body characters until we reach an
			// actual end-of-line
			type = newline;
			while (pos < end && '\n' != *pos) ++pos;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			type = number;
			while (pos < end && isdigit(*pos)) ++pos;
			break;
		default:
			if (isidentstart(c)) {
				type = symbol;
				while (pos < end && isidentbody(*pos)) ++pos;
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

iterator iterator::operator++(int)
{
	iterator temp(*this);
	++(*this);
	return temp;
}

bool iterator::operator==(const iterator &other) const
{
	return this->value == other.value && this->enditer == other.enditer;
}

bool iterator::operator!=(const iterator &other) const
{
	return !(*this == other);
}

bool iterator::match(token::id type) const
{
	return (value.begin != enditer) && (value.type == type);
}


} // namespace lexer


