// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#include "parser.h"
#include <memory>

using lexer::token;

// This is a simple LL(1) grammar so we do the obvious recursive descent.
// Each function represents one production. Its job is to consider the current
// input token and decide whether or not that token constitutes the beginning
// of its pattern. If no, the function should return nullptr and leave the
// input untouched. If yes, the function should consume the token, generate
// an output AST node, and proceed: if it fails to match later tokens, it
// must report a syntax error, try to resynchronize, and return whatever it
// was able to build.

bool parser::match(token::id id)
{
	if (current == id) {
		current = source? (++source)->type: token::eof;
		return true;
	} else {
		return false;
	}
}

bool parser::parens()
{
	return match(token::lparen) && compound() && match(token::rparen);
}

bool parser::brackets()
{
	return match(token::lbracket) && compound() && match(token::rbracket);
}

bool parser::braces()
{
	return match(token::lbrace) && procedure() && match(token::rbrace);
}

bool parser::group()
{
	// group: parens | brackets | braces
	return parens() || brackets() || braces();
}

bool parser::term()
{
	// term: <symbol> | <number> | group
	return match(token::symbol) || match(token::number) || group();
}

bool parser::subscript()
{
	// 	subscript: group
	return group();
}

bool parser::primary()
{
	// 	primary: term [subscript]
	if (!term()) {
		subscript();
		return true;
	}
	return false;
}

bool parser::infix()
{
	// infix: '&' | '|'
	return match(token::ampersand) || match(token::pipe);
}

bool parser::expression()
{
	// 	expression: primary [infix expression]
	do {
		if (!primary()) {
			return false;
		}
	} while (infix());
}

bool parser::compound()
{
	// compound: expression [',' compound]
	do {
		if (!expression()) {
			return false;
		}
	} while (match(token::comma));
}

bool parser::terminator()
{
	// terminator: ';' | <newline>
	return match(token::semicolon) || match(token::newline);
}

bool parser::assignment()
{
	// assignment: '=' expression
	return match(token::equals) && expression();
}

bool parser::invocation()
{
	// invocation: compound
	return compound();
}

bool parser::statement()
{
	// statement: <symbol> (assignment | invocation)
	return match(token::symbol) && (assignment() || invocation());
}

bool parser::procedure()
{
	// procedure: statement [terminator procedure]
	do {
		if (!statement()) {
			return false;
		}
	} while (terminator());
	return true;
}

parser::parser(lexer::iterator s, std::ostream &l):
	source(s), log(l)
{
	// input: [procedure] eof
	procedure();
	match(token::eof);
}

