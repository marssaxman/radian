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
