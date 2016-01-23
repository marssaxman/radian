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
#include "printer.h"

namespace {

using std::string;
using std::vector;
using std::deque;
using std::map;
using std::set;
using std::endl;

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
	virtual void leave(const dfg::block&) override;
	void emit(const dfg::node&, string op, vector<string> args);
	string sym(const dfg::node &n);
	string def(std::string name, const dfg::node &n);
	string makename(const dfg::node &n);
	map<const dfg::node*, string> names;
	set<std::string> used;
	string last_def;
	std::ostream &out;
};

void printer::visit(const dfg::error &n)
{
	def("!error!", n);
}

void printer::visit(const dfg::literal_int &n)
{
	std::stringstream ss;
	ss << std::hex << n.value;
	string s = ss.str();
	def(((s.size() & 1)? "$0": "$") + s, n);
}

void printer::visit(const dfg::param_val &n)
{
	def("*", n);
}

void printer::visit(const dfg::block_ref &n)
{
	def("_" + n.link_id, n);
}

void printer::leave(const dfg::unary &n)
{
	string op;
	switch (n.op) {
#define UNARY(x) case dfg::unary::x: op = #x; break;
#include "operators.h"
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
	def(index, n);
}

void printer::leave(const dfg::binary &n)
{
	string op;
	switch (n.op) {
#define BINARY(x) case dfg::binary::x: op = #x; break;
#include "operators.h"
	}
	emit(n, op, {sym(n.left), sym(n.right)});
}

void printer::leave(const dfg::ternary &n)
{
	string op;
	switch (n.op) {
#define TERNARY(x) case dfg::ternary::x: op = #x; break;
#include "operators.h"
	}
	emit(n, op, {sym(n.cond), sym(n.thenval), sym(n.elseval)});
}

void printer::leave(const dfg::variadic &n)
{
	string op;
	switch (n.op) {
#define VARIADIC(x) case dfg::variadic::x: op = #x; break;
#include "operators.h"
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
	last_def.clear();
}

void printer::leave(const dfg::block &b)
{
	out << std::right << std::setw(10) << " ";
	out << std::left << std::setw(6) << last_def << endl;
	last_def.clear();
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

string printer::sym(const dfg::node &n)
{
	return names[&n];
}

string printer::def(string name, const dfg::node &n)
{
	names[&n] = name;
	last_def = name;
	return name;
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
	return def("%" + name, n);
}

} // namespace

std::ostream& operator<<(std::ostream &out, const dfg::unit &src)
{
	printer p(out);
	src.accept(p);
	return out;
}

