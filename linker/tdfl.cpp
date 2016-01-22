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
#include "tdfl.h"

namespace tdfl {

struct builder
{
	size_t line_no = 0;
	std::ostream &err;
	builder(const block &s, std::ostream &errlog);
	dfg::node &literal(std::string);
	dfg::node &lookup(std::string);
	dfg::node &linkref(std::string);
	dfg::node &field(std::string);
	dfg::node &param(std::string);
	dfg::node &arg(std::string);
	void unary(int, std::string arg);
	void binary(int, std::string left, std::string right);
	void select(std::string c, std::string t, std::string f);
	void variadic(int, const std::vector<std::string> &args);
	void eval(const instruction &inst);
	dfg::builder dest;
	std::map<std::string, dfg::node*> symbols;
	std::reference_wrapper<dfg::node> result;
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
	dfg::node &tuple = arg(std::string(iter, s.end() - 1));
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

dfg::node &builder::arg(std::string s)
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
	default:
		err << line_no << ": illegal operand token \"" <<
				s << "\"" << std::endl;
	}
	return dest.make<dfg::error>();
}

void builder::unary(int opcode, std::string s)
{
	dfg::unary::opcode op = static_cast<dfg::unary::opcode>(opcode);
	dfg::node &source = arg(s);
	result = dest.make<dfg::unary>(op, source);
}

void builder::binary(int opcode, std::string l, std::string r)
{
	dfg::binary::opcode op = static_cast<dfg::binary::opcode>(opcode);
	dfg::node &left = arg(l);
	dfg::node &right = arg(r);
	result = dest.make<dfg::binary>(op, left, right);
}

void builder::select(std::string c, std::string t, std::string f)
{
	dfg::node &cond = arg(c);
	dfg::node &trueval = arg(t);
	dfg::node &elseval = arg(f);
	result = dest.make<dfg::select>(cond, trueval, elseval);
}

void builder::variadic(int opcode, const std::vector<std::string> &args)
{
	dfg::variadic::opcode op = static_cast<dfg::variadic::opcode>(opcode);
	std::vector<std::reference_wrapper<dfg::node>> sources;
	for (auto &s: args) {
		sources.push_back(arg(s));
	}
	result = dest.make<dfg::variadic>(op, sources);
}

void builder::eval(const instruction &inst)
{
	struct info
	{
		int arity;
		int opcode;
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
	auto langiter = language.find(inst.op);
	if (langiter == language.end()) {
		err << line_no << ": undefined instruction \"" <<
				inst.op << "\"" << std::endl;
		result = dest.make<dfg::error>();
		return;
	}
	struct info ii = langiter->second;
	std::vector<std::string> args = inst.args;
	if (ii.arity > 0 && ii.arity > args.size()) {
		err << line_no << ": not enough operands (expected " <<
				ii.arity << ", got " << args.size() << ")" << std::endl;
		args.resize(ii.arity);
	} else if (ii.arity > 0 && ii.arity < args.size()) {
		err << line_no << ": too many operands (expected " <<
				ii.arity << ", got " << args.size() << ")" << std::endl;
	}
	switch (ii.arity) {
		case 0: variadic(ii.opcode, args); break;
		case 1: unary(ii.opcode, args[0]); break;
		case 2: binary(ii.opcode, args[0], args[1]); break;
		case 3: select(args[0], args[1], args[2]); break;
	}
	if (!inst.def.empty()) {
		auto iter = symbols.find(inst.def);
		if (iter != symbols.end()) {
			err << line_no << ": redefinition of symbol \"" <<
					inst.def << "\"" << std::endl;
		}
		symbols[inst.def] = &result.get();
	}
}

builder::builder(const block &s, std::ostream &errlog):
	err(errlog),
	result(dest.param())
{
	for (auto &inst: s) {
		eval(inst);
		++line_no;
	}
}

dfg::block &&build(const block &src, std::ostream &errlog)
{
	return std::move(builder(src, errlog).dest.done());
}

} // namespace tdfl

