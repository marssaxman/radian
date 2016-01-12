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
		nil_ = 0,
		// atoms
		lit_, inp_, env_,
		// unary
		not_, test_,
		null_, size_, head_, tail_, last_,
		// binary
		add_, sub_, mul_, div_, quo_, rem_,
		shl_, shr_, and_, orl_, xor_,
		ceq_, cne_, clt_, cgt_, cle_, cge_,
		take_, drop_, item_,
		// ternary
		sel_,
		// variadic
		pack_, bind_, join_,
	} id;
	node(type t): id(t) {}
};

struct dummy: public node { dummy(): node(nil_) {} };
struct inp: public node { inp(): node(inp_) {} };
struct env: public node { env(): node(env_) {} };

struct literal: public node
{
	int64_t value;
	literal(int64_t v):
		node(lit_), value(v) {}
};

struct operation: public node
{
	std::vector<const node*> inputs;
	operation(type t, std::vector<const node*> &&i):
		node(t), inputs(std::move(i)) {}
};

class block
{
	std::vector<std::unique_ptr<node>> nodes;
public:
	template<typename T, typename... A>
	const node* add(A... a)
	{
		nodes.emplace_back(new T(a...));
		return nodes.back().get();
	}
	bool empty() const { return nodes.empty(); }
	const node* root() const { return nodes.back().get(); }
};

class unit
{
	std::vector<std::unique_ptr<block>> blocks;
public:
	unit() {}
	block *add();
};

} // namespace dfg

#endif //DFG_H

