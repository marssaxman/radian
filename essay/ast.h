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

#include "token.h"
#include <memory>
#include <deque>

namespace ast {

// each class of node represents some grammatical structure that the parser
// may find in the input. the parser will create nodes as appropriate when it
// matches patterns in the token stream. a node therefore represents a sequence
// of tokens, and the node class' job is to extract the information relevant to
// that pattern in a form which will be useful for later analysis.

struct visitor;

struct node
{
	lexer start;
	lexer end;
	virtual void accept(visitor&);
};

struct group: node
{
	std::unique_ptr<node> body;
	virtual void accept(visitor&) override;
};

struct sequence: node
{
	std::deque<std::unique_ptr<node>> body;
	virtual void accept(visitor&) override;
};

struct visitor
{
	virtual ~visitor() {}
	virtual void visit(const node&) {}
	virtual void enter(const group&) {}
	virtual void leave(const group&) {}
	virtual void enter(const sequence&) {}
	virtual void leave(const sequence&) {}
};

} // namespace ast

#endif //AST_H
