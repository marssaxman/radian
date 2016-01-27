// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#ifndef AST_H
#define AST_H

#include "lexer.h"

namespace ast {

struct node
{
	lexer::iterator begin;
	lexer::iterator end;
	node(lexer::iterator begin, lexer::iterator end);
};

struct group: node
{
	node *body;
	group(lexer::iterator begin, lexer::iterator end, node *b):
		node(begin, end), body(b) {}
};

struct parens: group
{
	parens(lexer::iterator begin, lexer::iterator end, node *body):
		group(begin, end, body) {}
};

struct brackets: group
{
	brackets(lexer::iterator begin, lexer::iterator end, node *body):
		group(begin, end, body) {}
};

struct braces: group
{
	braces(lexer::iterator begin, lexer::iterator end, node *body):
		group(begin, end, body) {}
};

} // namespace ast

#endif //AST_H
