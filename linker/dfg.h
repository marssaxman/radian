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

#ifndef DFG_H
#define DFG_H

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <map>

namespace dfg {

struct node
{
	enum type {
		error_ = 0,
		// atoms
		lit_, inp_, env_, reloc_,
		// unary
		not_, neg_,
		null_, size_, head_, tail_, last_,
		jump_,
		// binary
		add_, sub_, mul_, div_, quo_, rem_,
		shl_, shr_, and_, orl_, xor_,
		ceq_, cne_, clt_, cgt_, cle_, cge_,
		take_, drop_, item_,
		bind_, call_,
		// ternary
		sel_,
		// variadic
		pack_, join_,
	} id;
	node(type t): id(t) {}
};

struct dummy: public node { dummy(): node(error_) {} };
struct inp: public node { inp(): node(inp_) {} };
struct env: public node { env(): node(env_) {} };

struct literal: public node
{
	int64_t value;
	literal(int64_t v):
		node(lit_), value(v) {}
};

struct reloc: public node
{
	std::string name;
	reloc(std::string n):
		node(reloc_), name(n) {}
};

struct operation: public node
{
	std::vector<const node*> inputs;
	operation(type t, const std::vector<const node*> &i):
		node(t), inputs(i) {}
};

class unit
{
	std::vector<std::unique_ptr<node>> nodes;
	std::map<std::string, const node*> blocks;
public:
	unit() {}
	template<typename T, typename... A>
	const node* make(A... a)
	{
		nodes.emplace_back(new T(a...));
		return nodes.back().get();
	}
	void define(std::string name, const node *exp)
	{
		blocks[name] = exp;
	}
};

} // namespace dfg

#endif //DFG_H

