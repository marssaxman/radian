// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

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

