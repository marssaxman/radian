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

#include <memory>
#include <vector>
#include <utility>
#include <string>

namespace dfg { // data flow graph

struct visitor;

struct node
{
	virtual void accept(visitor&) const = 0;
	virtual ~node() {}
protected:
	node() {}
};

struct error: public node
{
	error(): node() {}
	virtual void accept(visitor &v) const override;
};

struct literal_int: public node
{
	literal_int(uint32_t v): node(), value(v) {}
	virtual void accept(visitor &v) const override;
	uint32_t value;
};

struct param_val: public node
{
	param_val(): node() {}
	virtual void accept(visitor &v) const override;
};

struct block_ref: public node
{
	block_ref(std::string id):
		node(), link_id(id) {}
	virtual void accept(visitor &v) const override;
	std::string link_id;
};

struct unary: public node
{
	enum opcode
	{
		notl,
		test,
		null,
		peek,
		next,
	};
	unary(opcode o, node &s): node(), op(o), source(s) {}
	virtual void accept(visitor &v) const override;
	opcode op;
	node &source;
};

struct field: public node
{
	field(node &s, uint32_t i):
		node(), source(s), index(i) {}
	virtual void accept(visitor &v) const override;
	node &source;
	uint32_t index;
};

struct binary: public node
{
	enum opcode
	{
		diff,
		xorl,
		item,
		head,
		skip,
		tail,
		drop,
	};
	binary(opcode o, node &l, node &r):
		node(), op(o), left(l), right(r) {}
	virtual void accept(visitor &v) const override;
	opcode op;
	node &left;
	node &right;
};

struct select: public node
{
	select(node &c, node &t, node &e):
		node(), cond(c), thenval(t), elseval(e) {}
	virtual void accept(visitor &v) const override;
	node &cond;
	node &thenval;
	node &elseval;
};

struct variadic: public node
{
	enum opcode
	{
		sum,
		all,
		any,
		equ,
		ord,
		asc,
		tuple,
		array,
		cat,
	};
	variadic(opcode o, const std::vector<std::reference_wrapper<node>> &s):
		node(), op(o), sources(s) {}
	virtual void accept(visitor &v) const override;
	opcode op;
	const std::vector<std::reference_wrapper<node>> sources;
};

struct visitor
{
	virtual ~visitor() {}
	virtual void visit(const error &n) {}
	virtual void visit(const literal_int &n) {}
	virtual void visit(const param_val &n) {}
	virtual void visit(const block_ref &n) {}
	virtual void visit(const unary &n) {}
	virtual void visit(const field &n) {}
	virtual void visit(const binary &n) {}
	virtual void visit(const select &n) {}
	virtual void visit(const variadic &n) {}
};

struct block
{
	block();
	template<typename T, typename... Args> node &make(Args&... args)
	{
		static_assert(!std::is_base_of<param_val, T>::value,
				"don't make new param_vals - use block.param()");
		T *out = new T(args...);
		code.emplace_back(out);
		return *out;
	}
	node &param() { return *code.front(); }
	void accept(visitor &v) const;
private:
	std::vector<std::unique_ptr<node>> code;
};



} // namespace dfg

#endif //DFG_H

