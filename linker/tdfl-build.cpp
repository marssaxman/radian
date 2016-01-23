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
#include <sstream>
#include <iomanip>
#include <assert.h>
#include <set>
#include "tdfl-build.h"

namespace tdfl {

using std::string;
using std::vector;
using std::deque;
using std::map;
using std::endl;

typedef std::reference_wrapper<dfg::node> node_ref;

struct builder
{
	size_t line_no = 0;
	std::ostream &err;
	builder(const code &s, std::ostream &errlog);
	void literal(string);
	void lookup(string);
	void linkref(string);
	void param_val(string);
	void field(string);
	void operand(string);
	void inst(string op, deque<string> &tokens);
	void def(string def, deque<string> &tokens);
	void eval(deque<string> &tokens);
	void parse(const string &line);
	std::ostream &report();
	dfg::unit unit;
	std::reference_wrapper<dfg::block> block;
	template<typename T, typename... Args> void make(Args&... args)
	{
		result = block.get().make<T>(args...);
	}
	void param()
	{
		result = block.get().param();
	}

	map<string, dfg::node*> symbols;
	std::reference_wrapper<dfg::node> result;
};

void builder::literal(string s)
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
			report() << "bogus numeric literal \"" << s << "\"" << endl;
		}
	}
	make<dfg::literal_int>(val);
}

void builder::lookup(string s)
{
	auto iter = symbols.find(s);
	if (iter != symbols.end()) {
		result = *iter->second;
	} else {
		report() << "reference to undefined symbol \"" << s << "\"";
	}
}

void builder::linkref(string s)
{
	make<dfg::block_ref>(s);
}

void builder::field(string s)
{
	// Collect digits until we reach a left paren, then evaluate whatever is
	// inside the parens and generate a reference to one of its fields.
	uint32_t index = 0;
	auto iter = s.begin();
	while (iter != s.end() && isdigit(*iter)) {
		index = index * 10 + s.front() - '0';
		++iter;
	}
	// If we've reached the end of the string, and the whole token was just
	// a number, that refers to a field of the parameter, which is usually a
	// tuple and so benefits from this convenient shorthand. Otherwise, we
	// expect to find a pair of parens and a subexpression.
	if (iter == s.end()) {
		param();
	} else if (*iter == '(' && s.back() == ')') {
		operand(string(iter, s.end() - 1));
	} else {
		report() << "expected a field reference token" << std::endl;
	}
	return make<dfg::field>(result, index);
}

void builder::param_val(string s)
{
	param();
	if (!s.empty()) {
		report() << "unexpected char in param token \"" << s << "\"" << endl;
	}
}

void builder::operand(string s)
{
	if (s.empty()) {
		report() << "expected a non-empty operand here" << endl;
		return;
	}
	string rest = s.substr(1);
	switch (s.front()) {
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': field(s); return;
		case '$': literal(rest); return;
		case '%': lookup(rest); return;
		case '_': linkref(rest); return;
		case '*': param_val(rest); return;
		case '^': if (rest.empty()) return;
		default: report() << "illegal operand token \"" << s << "\"" << endl;
	}
}

void builder::inst(string op, deque<string> &argtokens)
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
			dfg::ternary::opcode ter_op;
		};
	};
	static map<string, info> language = {
#define UNARY(x) {#x, {1, dfg::unary::x}},
#define BINARY(x) {#x, {2, dfg::binary::x}},
#define TERNARY(x) {#x, {3, dfg::ternary::x}},
#define VARIADIC(x) {#x, {0, dfg::variadic::x}},
#include "operators.h"
	};
	auto langiter = language.find(op);
	if (langiter == language.end()) {
		if (argtokens.empty()) {
			// Maybe this line is just referencing some value in order to
			// load up the previous-result value or define a symbol.
			operand(op);
		} else {
			report() << "undefined instruction \"" << op << "\"" << endl;
		}
		return;
	}
	struct info ii = langiter->second;
	vector<node_ref> val;
	for (auto &token: argtokens) {
		operand(token);
		val.push_back(result);
	}
	if (val.size() < ii.arity) {
		report() << "not enough operands (expected " << ii.arity <<
				", got " << argtokens.size() << ")" << endl;
		do {
			val.push_back(result);
		} while (val.size() < ii.arity);
	} else if (ii.arity > 0 && argtokens.size() > ii.arity) {
		report() << "too many operands (expected " << ii.arity <<
				", got " << argtokens.size() << ")" << endl;
	}
	switch (ii.arity) {
		case 0: make<dfg::variadic>(ii.var_op, val); break;
		case 1: make<dfg::unary>(ii.un_op, val[0]); break;
		case 2: make<dfg::binary>(ii.bin_op, val[0], val[1]); break;
		case 3: make<dfg::ternary>(ii.ter_op, val[0], val[1], val[2]); break;
		default: make<dfg::error>(); break;
	}
}

void builder::def(string def, deque<string> &tokens)
{
	if (def.front() == '_') {
		// This declaration begins a new block.
		block = unit.make(def.substr(1));
		symbols.clear();
		param();
		if (!tokens.empty()) {
			std::string name = tokens.front();
			tokens.pop_front();
			inst(name, tokens);
		}
	} else if (!tokens.empty()) {
		// This is a local symbol declaration inside the same block.
		string name = tokens.front();
		tokens.pop_front();
		inst(name, tokens);
		auto iter = symbols.find(def);
		if (iter == symbols.end()) {
			symbols[def] = &result.get();
		} else {
			report() << "redefinition of symbol \"" << def << "\"" << endl;
		}
	} else {
		report() << "definition with no value" << endl;
	}
}

void builder::eval(deque<string> &tokens)
{
	if (tokens.empty()) {
		return;
	}
	string front = tokens.front();
	tokens.pop_front();
	if (front.back() == ':') {
		front.pop_back();
		def(front, tokens);
	} else {
		inst(front, tokens);
	}
}

void builder::parse(const string &line)
{
	deque<string> tokens;
	std::istringstream input(line);
	std::string token;
	while (input >> std::skipws >> token) {
		if (token.empty()) {
			continue;
		}
		if (token.front() == '#') {
			// comment
			break;
		}
		if (token.back() == ',') {
			// optional argument separator
			token.pop_back();
		}
		tokens.push_back(token);
	}
	eval(tokens);
}

std::ostream &builder::report()
{
	err << line_no << ": ";
	make<dfg::error>();
	return err;
}

builder::builder(const code &s, std::ostream &errlog):
	err(errlog),
	block(unit.make("start")),
	result(block.get().param())
{
	for (auto &line: s) {
		parse(line);
		++line_no;
	}
}

dfg::unit build(const code &src, std::ostream &errlog)
{
	return builder(src, errlog).unit;
}

} // namespace tdfl

