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

#include "dfg.h"

const dfg::node *dfg::reader::top()
{
	if (stack.empty()) {
		fail("stack underflow");
		add(new dummy);
	}
	return stack.back();
}

const dfg::node *dfg::reader::pop()
{
	auto out = top();
	stack.pop_back();
	return out;
}

void dfg::reader::push(const dfg::node *n)
{
	stack.push_back(n);
}

dfg::node *const dfg::reader::make(dfg::node *const n)
{
	body.emplace_back(std::unique_ptr<node>(n));
	return n;
}

void dfg::reader::add(dfg::node *const n)
{
	push(make(n));
}

void dfg::reader::fail(std::string msg)
{
	using namespace std;
	err << dec << loc.line << ":" << loc.column << ": " << msg << endl;
}

void dfg::reader::literal(std::string text)
{
	if (text.size() > 16) {
		fail("oversize literal");
	}
	int64_t value = 0;
	for (char c: text) {
		value <<= 4;
		if (c >= '0' && c <= '9') {
			value += (c - '0') + 0x0;
		} else if (c >= 'a' && c <= 'f') {
			value += (c - 'a') + 0xA;
		} else if (c >= 'A' && c <= 'F') {
			value += (c - 'A') + 0xA;
		} else {
			fail("illegal hex digit '" + std::string(1, c) + "'");
			break;
		}
	}
	add(new dfg::literal(value));
}

void dfg::reader::symbol(std::string name)
{
	auto iter = symbols.find(name);
	const dfg::node *out = nullptr;
	if (iter != symbols.end()) {
		out = iter->second;
	} else {
		fail("undefined symbol \"" + name + "\"");
		out = make(new dummy);
		symbols[name] = out;
	}
	push(out);
}

void dfg::reader::unop(node::type id)
{
	add(new dfg::unop(id, pop()));
}

void dfg::reader::binop(node::type id)
{
	add(new dfg::binop(id, pop(), pop()));
}

void dfg::reader::ternop(node::type id)
{
	add(new dfg::ternop(id, pop(), pop(), pop()));
}

void dfg::reader::compound(node::type id)
{
}

void dfg::reader::token(std::string text)
{
	// Figure out what an input token means. 
	// Tokens which begin with a punctuation character have special meaning;
	// other tokens are instruction names.
	char c = text.front();
	if (ispunct(c)) {
		std::string body = text.substr(1);
		switch (c) {
			case '$': literal(body); break;
			case '%': symbol(body); break;
			default: fail("unknown token prefix '" + std::string(1, c) + "'");
		}
	} else {
		if (text == "add") {
			binop(node::add_);
		}
		fail("undefined operation \"" + text + "\"");
	}
}

void dfg::reader::instruction(std::string text)
{
	// The body of an instruction consists of operands proceeded by a mnemonic.
	// Process the tokens right to left, evaluating them with respect to the
	// stack.
	size_t end = text.size();
	while (end > 0) {
		// Find the length of the token character sequence, if present.
		size_t start = end;
		do {
			char c = text[start - 1];
			if (isblank(c) || !isgraph(c)) break;
			--start;
		} while (start > 0);
		if (start < end) {
			// If we found a token, go process it.
			loc.column = start;
			token(text.substr(start, end - start));
			end = start;
		} else {
			// If what we found was not a token, skip it.
			--end;
		}
	}
}

void dfg::reader::line(std::string text)
{
	// If the line begins with a token ending with a colon, the token is a
	// symbol definition against the instruction result.
	size_t start = 0;
	while (start < text.size() && isblank(text[start])) {
		++start;
	}
	std::string defsym;
	size_t pos = text.find_first_of(':', start);
	if (pos != std::string::npos) {
		defsym = text.substr(start, pos - start);
		start = pos + 1;
	}
	// If the line ends with text following a semicolon, that is a comment,
	// which we will ignore.
	size_t end = text.size();
	for (size_t i = start; i < end; ++i) {
		if (';' == text[i]) {
			end = i;
		}
	}
	instruction(text.substr(start, end - start));
	if (!defsym.empty()) {
		if (symbols.find(defsym) != symbols.end()) {
			fail("redefined symbol \"" + defsym + "\"");
			return;
		}
		symbols[defsym] = top();
	}
}

dfg::reader::reader(dfg &_dest, std::istream &src, std::ostream &_err):
	err(_err),
	dest(_dest),
	instructions({
		{"not", [=](){unop(node::not_);}},
		{"add", [=](){binop(node::add_);}},
		{"sub", [=](){binop(node::sub_);}},
		{"mul", [=](){binop(node::mul_);}},
		{"div", [=](){binop(node::div_);}},
		{"quo", [=](){binop(node::quo_);}},
		{"rem", [=](){binop(node::rem_);}},
		{"shl", [=](){binop(node::shl_);}},
		{"shr", [=](){binop(node::shr_);}},
		{"and", [=](){binop(node::and_);}},
		{"orl", [=](){binop(node::orl_);}},
		{"xor", [=](){binop(node::xor_);}},
		{"ceq", [=](){binop(node::ceq_);}},
		{"cne", [=](){binop(node::cne_);}},
		{"clt", [=](){binop(node::clt_);}},
		{"cgt", [=](){binop(node::cgt_);}},
		{"cle", [=](){binop(node::cle_);}},
		{"cge", [=](){binop(node::cge_);}},
		{"sel", [=](){ternop(node::sel_);}},
	})
{
	// A serialized DFG is a text file composed of lines containing tokens.
	// A line consists of a sequence of zero or more tokens followed by an
	// optional comment, delimited by ';', which continues to end of line.
	// Tokens are sequences of isgraph() delimited by sequences of isspace().
	// Lines are evaluated top to bottom, tokens are evaluated right to left.
	// We will extract the tokens from this stream and eval() them in order.
	loc.line = 1;
	for (std::string text; std::getline(src, text); ++loc.line) {
		line(text);
	}
}

void dfg::read(std::istream &src, std::ostream &err)
{
	reader(*this, src, err);
}

