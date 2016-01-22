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
#include "tdfl.h"

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
		{"jump", {1, dfg::unary::jump}},
		{"notl", {1, dfg::unary::notl}},
		{"test", {1, dfg::unary::test}},
		{"null", {1, dfg::unary::null}},
		{"peek", {1, dfg::unary::peek}},
		{"next", {1, dfg::unary::next}},
		{"call", {2, dfg::binary::call}},
		{"bind", {2, dfg::binary::bind}},
		{"diff", {2, dfg::binary::diff}},
		{"xorl", {2, dfg::binary::xorl}},
		{"item", {2, dfg::binary::item}},
		{"head", {2, dfg::binary::head}},
		{"skip", {2, dfg::binary::skip}},
		{"tail", {2, dfg::binary::tail}},
		{"drop", {2, dfg::binary::drop}},
		{"bcc", {3, dfg::ternary::bcc}},
		{"sel", {3, dfg::ternary::sel}},
		{"loop", {3, dfg::ternary::loop}},
		{"sum", {0, dfg::variadic::sum}},
		{"andl", {0, dfg::variadic::andl}},
		{"orl", {0, dfg::variadic::orl}},
		{"cpeq", {0, dfg::variadic::cpeq}},
		{"cpge", {0, dfg::variadic::cpge}},
		{"cpgt", {0, dfg::variadic::cpgt}},
		{"tuple", {0, dfg::variadic::tuple}},
		{"array", {0, dfg::variadic::array}},
		{"cat", {0, dfg::variadic::cat}},
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

struct printer: public dfg::visitor
{
	printer(std::ostream &o):
		out(o) {}
	virtual void visit(const dfg::error&) override;
	virtual void visit(const dfg::literal_int&) override;
	virtual void visit(const dfg::param_val&) override;
	virtual void visit(const dfg::block_ref&) override;
	virtual void leave(const dfg::unary&) override;
	virtual void leave(const dfg::field&) override;
	virtual void leave(const dfg::binary&) override;
	virtual void leave(const dfg::ternary&) override;
	virtual void leave(const dfg::variadic&) override;
	virtual void enter(const dfg::block&) override;
	void emit(const dfg::node&, std::string op, std::vector<std::string> args);
	std::string sym(const dfg::node &n) { return names[&n]; }
	std::string makename(const dfg::node &n);
	std::map<const dfg::node*, std::string> names;
	std::set<std::string> used;
	std::ostream &out;
};

void printer::visit(const dfg::error &n)
{
	names[&n] = "!error!";
}

void printer::visit(const dfg::literal_int &n)
{
	std::stringstream ss;
	ss << std::hex << n.value;
	string s = ss.str();
	names[&n] = ((s.size() & 1)? "$0": "$") + s;
}

void printer::visit(const dfg::param_val &n)
{
	names[&n] = "*";
}

void printer::visit(const dfg::block_ref &n)
{
	names[&n] = "_" + n.link_id;
}

void printer::leave(const dfg::unary &n)
{
	string op;
	switch (n.op) {
		case dfg::unary::jump: op = "jump"; break;
		case dfg::unary::notl: op = "notl"; break;
		case dfg::unary::test: op = "test"; break;
		case dfg::unary::null: op = "null"; break;
		case dfg::unary::peek: op = "peek"; break;
		case dfg::unary::next: op = "next"; break;
	}
	emit(n, op, {sym(n.source)});
}

void printer::leave(const dfg::field &n)
{
	string source = sym(n.source);
	std::stringstream ss;
	ss << std::dec << n.index;
	string index = ss.str();
	if (source != "*") {
		index += "(" + source + ")";
	}
	names[&n] = index;
}

void printer::leave(const dfg::binary &n)
{
	string op;
	switch (n.op) {
		case dfg::binary::call: op = "call"; break;
		case dfg::binary::bind: op = "bind"; break;
		case dfg::binary::diff: op = "diff"; break;
		case dfg::binary::xorl: op = "xorl"; break;
		case dfg::binary::item: op = "item"; break;
		case dfg::binary::head: op = "head"; break;
		case dfg::binary::skip: op = "skip"; break;
		case dfg::binary::tail: op = "tail"; break;
		case dfg::binary::drop: op = "drop"; break;
	}
	emit(n, op, {sym(n.left), sym(n.right)});
}

void printer::leave(const dfg::ternary &n)
{
	string op;
	switch (n.op) {
		case dfg::ternary::bcc: op = "bcc"; break;
		case dfg::ternary::sel: op = "sel"; break;
		case dfg::ternary::loop: op = "loop"; break;
	}
	emit(n, op, {sym(n.cond), sym(n.thenval), sym(n.elseval)});
}

void printer::leave(const dfg::variadic &n)
{
	string op;
	switch (n.op) {
		case dfg::variadic::sum: op = "sum"; break;
		case dfg::variadic::andl: op = "andl"; break;
		case dfg::variadic::orl: op = "orl"; break;
		case dfg::variadic::cpeq: op = "cpeq"; break;
		case dfg::variadic::cpge: op = "cpge"; break;
		case dfg::variadic::cpgt: op = "cpgt"; break;
		case dfg::variadic::tuple: op = "tuple"; break;
		case dfg::variadic::array: op = "array"; break;
		case dfg::variadic::cat: op = "cat"; break;
	}
	vector<string> tokens;
	for (auto &i: n.sources) {
		tokens.push_back(sym(i.get()));
	}
	emit(n, op, tokens);
}

void printer::enter(const dfg::block &b)
{
	out << "_" << b.link_id() << ":" << std::endl;
	names.clear();
	used.clear();
}

void printer::emit(const dfg::node &n, string op, vector<string> args)
{
	if (names.find(&n) != names.end()) {
		// we've already printed this node; no need to do it again
		return;
	}
	out << std::right << std::setw(10) << makename(n) + ": ";
	out << std::left << std::setw(6) << op << std::setw(1);
	for (auto iter = args.begin(); iter != args.end();) {
		out << *iter;
		if (++iter != args.end()) {
			out << ", ";
		}
	}
	out << std::endl;
}

string printer::makename(const dfg::node &n)
{
	// Generate a name for this node based on its address. We'll shift the
	// address over by 4 since allocations are always word-aligned, then use
	// enough bytes of the address to guarantee uniqueness.
	static vector<string> koremutake;
	if (koremutake.empty()) {
		koremutake.resize(128, "");
		const char *i =
			"BA BE BI BO BU BY DA DE DI DO DU DY FA FE FI FO FU FY GA GE GI "
			"GO GU GY HA HE HI HO HU HY JA JE JI JO JU JY KA KE KI KO KU KY "
			"LA LE LI LO LU LY MA ME MI MO MU MY NA NE NI NO NU NY PA PE PI "
			"PO PU PY RA RE RI RO RU RY SA SE SI SO SU SY TA TE TI TO TU TY "
			"VA VE VI VO VU VY BRA BRE BRI BRO BRU BRY DRA DRE DRI DRO DRU "
			"DRY FRA FRE FRI FRO FRU FRY GRA GRE GRI GRO GRU GRY PRA PRE PRI "
			"PRO PRU PRY STA STE STI STO STU STY TRA TRE ";
		for (auto &str: koremutake) {
			const char *end = i;
			while (' ' != *end) ++end;
			str = std::string(i, end - i);
			i = ++end;
		}
	}
	uintptr_t u = reinterpret_cast<uintptr_t>(&n) >> 2;
	string name;
	do {
		name += koremutake[u & 0x7F];
		u >>= 7;
	} while (used.find(name) != used.end());
	used.insert(name);
	names[&n] = "%" + name;
	return name;
}

void print(const dfg::unit &src, std::ostream &out)
{
	printer p(out);
	src.accept(p);
}

} // namespace tdfl

