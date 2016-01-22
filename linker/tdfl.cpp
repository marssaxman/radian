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

#include <map>
#include <deque>
#include "tdfl.h"

namespace tdfl {

typedef std::reference_wrapper<dfg::node> node_ref;

struct builder
{
	size_t line_no = 0;
	std::ostream &err;
	builder(const code &s, std::ostream &errlog);
	dfg::node &literal(std::string);
	dfg::node &lookup(std::string);
	dfg::node &linkref(std::string);
	dfg::node &field(std::string);
	dfg::node &param(std::string);
	dfg::node &operand(std::string);
	dfg::node &inst(std::string op, std::deque<std::string> &tokens);
	void def(std::string def, std::deque<std::string> &tokens);
	void eval(std::deque<std::string> &tokens);
	dfg::builder dest;
	std::map<std::string, dfg::node*> symbols;
	node_ref result;
};

dfg::node &builder::literal(std::string s)
{
	uint32_t val = 0;
	for (char c: s) {
		val = val << 4;
		if (c >= 0 && c <= '9') {
			val += c - '0';
		} else if (c >= 'A' && c <= 'F') {
			val += c - 'A' + 0xA;
		} else if (c >= 'a' && c <= 'f') {
			val += c - 'a' + 0xA;
		} else {
			err << line_no << ": bogus numeric literal \"" <<
					s << "\"" << std::endl;
			return dest.make<dfg::error>();
		}
	}
	return dest.make<dfg::literal_int>(val);
}

dfg::node &builder::lookup(std::string s)
{
	auto iter = symbols.find(s);
	if (iter == symbols.end()) {
		err << line_no << ": reference to undefined symbol \"" <<
				s << "\"" << std::endl;
		return dest.make<dfg::error>();
	} else {
		return *iter->second;
	}
}

dfg::node &builder::linkref(std::string s)
{
	return dest.make<dfg::block_ref>(s);
}

dfg::node &builder::field(std::string s)
{
	// Collect digits until we reach a left paren, then evaluate whatever is
	// inside the parens and generate a reference to one of its fields.
	uint32_t index = 0;
	auto iter = s.begin();
	while (iter != s.end() && isdigit(*iter)) {
		index = index * 10 + s.front() - '0';
		++iter;
	}
	// The next char should be a left paren and the last char should be a
	// right paren, and the text in between should be an operand.
	if (iter == s.end() || *iter != '(' || s.back() != ')') {
		err << line_no << ": expected a field reference token" << std::endl;
		return dest.make<dfg::error>();
	}
	dfg::node &tuple = operand(std::string(iter, s.end() - 1));
	return dest.make<dfg::field>(tuple, index);
}

dfg::node &builder::param(std::string s)
{
	if (s.empty()) {
		return dest.param();
	}
	uint32_t index = 0;
	for (char c: s) {
		if (isdigit(c)) {
			index = index * 10 + c - '0';
		} else {
			err << line_no << ": unexpected non-digit char in param \"" <<
					s << "\"" << std::endl;
			return dest.make<dfg::error>();
		}
	}
	return dest.make<dfg::field>(dest.param(), index);
}

dfg::node &builder::operand(std::string s)
{
	if (s.empty()) {
		err << line_no << ": expected a non-empty operand here" << std::endl;
		return dest.make<dfg::error>();
	}
	std::string rest = s.substr(1);
	switch (s.front()) {
		case '$': return literal(rest);
		case '%': return lookup(rest);
		case '_': return linkref(rest);
		case '@': return field(rest);
		case '*': return param(rest);
		case '^': if (rest.empty()) return result;
		default: err << line_no << ": illegal operand token \"" <<
				s << "\"" << std::endl;
	}
	return dest.make<dfg::error>();
}

dfg::node &builder::inst(std::string op, std::deque<std::string> &argtokens)
{
	struct info
	{
		int arity;
		union
		{
			int opcode;
			dfg::variadic::opcode var_op;
			dfg::unary::opcode un_op;
			dfg::binary::opcode bin_op;
		};
	};
	static std::map<std::string, info> language = {
		{"notl", {1, dfg::unary::notl}},
		{"test", {1, dfg::unary::test}},
		{"null", {1, dfg::unary::null}},
		{"peek", {1, dfg::unary::peek}},
		{"next", {1, dfg::unary::next}},
		{"diff", {2, dfg::binary::diff}},
		{"xorl", {2, dfg::binary::xorl}},
		{"item", {2, dfg::binary::item}},
		{"head", {2, dfg::binary::head}},
		{"skip", {2, dfg::binary::skip}},
		{"tail", {2, dfg::binary::tail}},
		{"drop", {2, dfg::binary::drop}},
		{"cond", {3}},
		{"sum", {0, dfg::variadic::sum}},
		{"all", {0, dfg::variadic::all}},
		{"any", {0, dfg::variadic::any}},
		{"equ", {0, dfg::variadic::equ}},
		{"ord", {0, dfg::variadic::ord}},
		{"asc", {0, dfg::variadic::asc}},
		{"tuple", {0, dfg::variadic::tuple}},
		{"array", {0, dfg::variadic::array}},
		{"cat", {0, dfg::variadic::cat}},
	};
	auto langiter = language.find(op);
	if (langiter == language.end()) {
		if (argtokens.empty()) {
			// Maybe this line is just referencing some value in order to
			// load up the previous-result value or define a symbol.
			return operand(op);
		}
		err << line_no << ": undefined instruction \"" <<
				op << "\"" << std::endl;
		return dest.make<dfg::error>();
	}
	struct info ii = langiter->second;
	std::vector<node_ref> vals;
	if (argtokens.size() < ii.arity) {
		// The leftmost operand may be ommitted, implicitly referencing the
		// result of the previous operation.
		vals.push_back(result);
	}
	for (auto &token: argtokens) {
		vals.push_back(operand(token));
	}
	if (vals.size() < ii.arity) {
		err << line_no << ": not enough operands (expected " << ii.arity <<
				", got " << argtokens.size() << ")" << std::endl;
		do {
			vals.push_back(dest.make<dfg::error>());
		} while (vals.size() < ii.arity);
	} else if (ii.arity > 0 && argtokens.size() > ii.arity) {
		err << line_no << ": too many operands (expected " << ii.arity <<
				", got " << argtokens.size() << ")" << std::endl;
	}
	switch (ii.arity) {
		case 0: return dest.make<dfg::variadic>(ii.var_op, vals);
		case 1: return dest.make<dfg::unary>(ii.un_op, vals[0]);
		case 2: return dest.make<dfg::binary>(ii.bin_op, vals[0], vals[1]);
		case 3: return dest.make<dfg::select>(vals[0], vals[1], vals[2]);
		default: return dest.make<dfg::error>();
	}
}

void builder::def(std::string def, std::deque<std::string> &tokens)
{
	if (tokens.empty()) {
		err << line_no << ": definition with no value" << std::endl;
		return;
	}
	std::string name = tokens.front();
	tokens.pop_front();
	inst(name, tokens);
	auto iter = symbols.find(def);
	if (iter != symbols.end()) {
		err << line_no << ": redefinition of symbol \"" <<
				def << "\"" << std::endl;
	}
	symbols[def] = &result.get();
}

void builder::eval(std::deque<std::string> &tokens)
{
	if (tokens.empty()) {
		return;
	}
	std::string front = tokens.front();
	tokens.pop_front();
	if (front.back() == ':') {
		def(front, tokens);
	} else {
		inst(front, tokens);
	}
}

builder::builder(const code &s, std::ostream &errlog):
	err(errlog),
	result(dest.param())
{
	for (auto &line: s) {
		std::deque<std::string> tokens;
		auto begin = line.begin();
		for(;;) {
			while (isspace(*begin) && begin != line.end()) {
				++begin;
			}
			auto end = begin;
			while (!isspace(*end) && end != line.end()) {
				++end;
			}
			if (end == begin) {
				break;
			}
			tokens.emplace_back(begin, end);
			begin = end;
		}
		eval(tokens);
		++line_no;
	}
}

dfg::block &&build(const code &src, std::ostream &errlog)
{
	return std::move(builder(src, errlog).dest.done());
}

} // namespace tdfl

