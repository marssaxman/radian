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

class dfg
{
public:
	class node
	{
	public:
		enum type {
			nil_ = 0,
			// atoms
			lit_,
			// unary
			not_,
			// binary
			add_, sub_, mul_, div_, quo_, rem_,
			shl_, shr_, and_, orl_, xor_,
			ceq_, cne_, clt_, cgt_, cle_, cge_,
			// ternary
			sel_,
		};
		type id() const { return t; }
		virtual ~node() {}
	protected:
		node(type _t): t(_t) {}
	private:
		type t;
	};
	class dummy: public node
	{
	public:
		dummy(): node(nil_) {}
	};
	class literal: public node
	{
		int64_t value;
	public:
		literal(int64_t v): node(lit_), value(v) {}
	};
	class unop: public node
	{
		const node *v;
	public:
		unop(type _t, const node *_v): node(_t), v(_v) {}
	};
	class binop: public node
	{
		const node *l;
		const node *r;
	public:
		binop(type _t, const node *_l, const node *_r):
			node(_t), l(_l), r(_r) {}
	};
	class ternop: public node
	{
		const node *w;
		const node *t;
		const node *f;
	public:
		ternop(type _i, const node *_w, const node *_t, const node *_f):
			node(_i), w(_w), t(_t), f(_f) {}
	};
	void read(std::istream &src, std::ostream &log);
private:
	class reader
	{
		struct {
			size_t line = 1;
			size_t column = 0;
		} loc;
		std::ostream &err;
		dfg &dest;
		std::vector<std::unique_ptr<node>> body;
		std::vector<const node*> stack;
		std::map<std::string, const node*> symbols;
		std::map<std::string, std::function<void()>> instructions;
		const node *top();
		const node *pop();
		void push(const node*);
		node*const make(node*const);
		void add(node*const);
		void fail(std::string message);
		void literal(std::string body);
		void symbol(std::string name);
		void token(std::string token);
		void instruction(std::string text);
		void unop(node::type id);
		void binop(node::type id);
		void ternop(node::type id);
		void compound(node::type id);
		void line(std::string text);
	public:
		reader(dfg &dest, std::istream &src, std::ostream &err);
	};
};

#endif //DFG_H

