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
	literal_int(uint32_t v): node(), _value(v) {}
	virtual void accept(visitor &v) const override;
	uint32_t _value;
};

struct param_val: public node
{
	param_val(): node() {}
	virtual void accept(visitor &v) const override;
};

struct block_ref: public node
{
	block_ref(std::string id): node(), _link_id(id) {}
	virtual void accept(visitor &v) const override;
	std::string _link_id;
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
	unary(opcode o, node &s): node(), _op(o), _source(s) {}
	virtual void accept(visitor &v) const override;
	opcode _op;
	node &_source;
};

struct field: public node
{
	field(node &s, uint32_t i):
		node(), _source(s), _index(i) {}
	virtual void accept(visitor &v) const override;
	node &_source;
	uint32_t _index;
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
		node(), _op(o), _left(l), _right(r) {}
	virtual void accept(visitor &v) const override;
	opcode _op;
	node &_left;
	node &_right;
};

struct select: public node
{
	select(node &c, node &t, node &e):
		node(), _cond(c), _then_val(t), _else_val(e) {}
	virtual void accept(visitor &v) const override;
	node &_cond;
	node &_then_val;
	node &_else_val;
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
		node(), _op(o), _sources(s) {}
	virtual void accept(visitor &v) const override;
	opcode _op;
	std::vector<std::reference_wrapper<node>> _sources;
};

struct visitor
{
	virtual ~visitor() {}
	virtual void visit(const error&) {}
	virtual void visit(const literal_int&) {}
	virtual void visit(const param_val&) {}
	virtual void visit(const block_ref&) {}
	virtual void visit(const unary&) {}
	virtual void visit(const field&) {}
	virtual void visit(const binary&) {}
	virtual void visit(const select&) {}
	virtual void visit(const variadic&) {}
};

struct block
{
	block(std::vector<std::unique_ptr<node>> &&s): _code(std::move(s)) {}
	void accept(visitor &v) { _code.back()->accept(v); }
private:
	std::vector<std::unique_ptr<node>> _code;
};

struct builder
{
	builder();
	template<typename T, typename... Args> node &make(Args&... args)
	{
		static_assert(!std::is_base_of<param_val, T>::value,
				"don't make new param_vals - use builder.param()");
		_code.emplace_back(new T(args...));
		return *_code.back();
	}
	node &param() { return _param; }
	block &&done() { return std::move(block(std::move(_code))); }
private:
	node &_param;
	std::vector<std::unique_ptr<node>> _code;
};

} // namespace dfg

#endif //DFG_H

