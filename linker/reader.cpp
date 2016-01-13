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

#include "reader.h"

void dfg::reader::fail(std::string msg)
{
	using namespace std;
	err << dec << loc.line << ":" << loc.column << ": " << msg << endl;
}

const dfg::node *dfg::reader::error(std::string msg)
{
	fail(msg);
	return dest.make<dummy>();
}

const dfg::node *dfg::reader::literal(std::string text)
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
	return dest.make<dfg::literal>(value);
}

const dfg::node *dfg::reader::symbol(std::string name)
{
	auto iter = symbols.find(name);
	if (iter != symbols.end()) {
		return iter->second;
	}
	return error("undefined symbol \"" + name + "\"");
}

const dfg::node *dfg::reader::reloc(std::string name)
{
	return dest.make<dfg::reloc>(name);
}

const dfg::node *dfg::reader::terminal(char c, std::string body)
{
	switch (c) {
		case '$': return literal(body); break;
		case '%': return symbol(body); break;
		case '@': return reloc(body); break;
		default: return error("unknown prefix '" + std::string(1, c) + "'");
	}
}

const dfg::node *dfg::reader::operation(std::string text)
{
	struct op {
		enum {
			nullary = 0,
			unary = 1,
			binary = 2,
			ternary = 3,
			variadic = -1
		} arity;
		node::type type;
	};
	static std::map<std::string, op> operators = {
		{"inp", {op::nullary, node::inp_}},
		{"env", {op::nullary, node::env_}},
		{"not", {op::unary, node::not_}},
		{"neg", {op::unary, node::neg_}},
		{"null", {op::unary, node::null_}},
		{"size", {op::unary, node::size_}},
		{"head", {op::unary, node::head_}},
		{"tail", {op::unary, node::tail_}},
		{"last", {op::unary, node::last_}},
		{"jump", {op::unary, node::jump_}},
		{"add", {op::binary, node::add_}},
		{"sub", {op::binary, node::sub_}},
		{"mul", {op::binary, node::mul_}},
		{"div", {op::binary, node::div_}},
		{"quo", {op::binary, node::quo_}},
		{"rem", {op::binary, node::rem_}},
		{"shl", {op::binary, node::shl_}},
		{"shr", {op::binary, node::shr_}},
		{"and", {op::binary, node::and_}},
		{"orl", {op::binary, node::orl_}},
		{"xor", {op::binary, node::xor_}},
		{"ceq", {op::binary, node::ceq_}},
		{"cne", {op::binary, node::cne_}},
		{"clt", {op::binary, node::clt_}},
		{"cgt", {op::binary, node::cgt_}},
		{"cle", {op::binary, node::cle_}},
		{"cge", {op::binary, node::cge_}},
		{"take", {op::binary, node::take_}},
		{"drop", {op::binary, node::drop_}},
		{"item", {op::binary, node::item_}},
		{"bind", {op::binary, node::bind_}},
		{"call", {op::binary, node::call_}},
		{"sel", {op::ternary, node::sel_}},
		{"pack", {op::variadic, node::pack_}},
		{"join", {op::variadic, node::join_}},
	};
	auto opiter = operators.find(text);
	if (opiter == operators.end()) {
		return error("undefined operation \"" + text + "\"");
	}
	auto which = opiter->second;
	// retrieve arguments for this operation according to its arity
	int64_t argc = 0;
	if (op::variadic == which.arity) {
		if (stack.empty()) {
			return error("stack underflow; variadic operator requires count");
		}
		if (stack.back()->id != node::lit_) {
			return error("variadic operator count must be integer literal");
		}
		argc = static_cast<const dfg::literal*>(stack.back())->value;
		if (argc < 0) {
			return error("variadic operator count must not be negative");
		}
		if (argc > stack.size()) {
			return error("stack underflow: not enough values available");
		}
	} else {
		argc = which.arity;
	}
	auto argiter = stack.end() - argc;
	std::vector<const node *> inputs(argiter, stack.end());
	stack.erase(argiter, stack.end());
	return dest.make<dfg::operation>(which.type, inputs);
}

const dfg::node *dfg::reader::token(std::string text)
{
	// Figure out what an input token means. 
	// Tokens which begin with a punctuation character have special meaning;
	// other tokens are instruction names.
	char c = text.front();
	if (ispunct(c)) {
		return terminal(c, text.substr(1));
	} else {
		return operation(text);
	}
}

void dfg::reader::eval(std::string text)
{
	// Evaluate the body of this line, processing tokens from right to left.
	// Each token may push and/or pop values from the stack, depending on
	// its type. We go from right to left because we read instructions from
	// left to right, and each sub-expression provides input for the one which
	// contains it.
	size_t end = text.size();
	while (end > 0) {
		// Find the length of the token character sequence, if present.
		size_t start = end;
		do {
			char c = text[start - 1];
			if (isblank(c) || !isgraph(c)) break;
			--start;
		} while (start > 0);
		size_t len = end - start;
		if (len > 0) {
			// If we found a token, go process it.
			loc.column = end = start;
			stack.push_back(token(text.substr(start, len)));
		} else {
			// If what we found was not a token, skip it.
			--end;
		}
	}
}

void dfg::reader::stmt(std::string text)
{
	// A statement may have label, a body, and/or a comment.
	// If the first token on the line ends with a colon, that token is a
	// local symbol definition.
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
	// If the line contains a semicolon, all that follows is a comment, which
	// we will ignore.
	size_t end = text.size();
	for (size_t i = start; i < end; ++i) {
		if (';' == text[i]) {
			end = i;
		}
	}
	// What remains, if anything, is the body of the line.
	eval(text.substr(start, end - start));
	// If we started with a definition, assign the result of the evaluation
	// to that symbol.
	if (!defsym.empty()) {
		if (symbols.find(defsym) != symbols.end()) {
			fail("redefined symbol \"" + defsym + "\"");
		}
		if (!stack.empty()) {
			symbols[defsym] = stack.back();
		} else {
			fail("stack underflow");
		}
	}
}

void dfg::reader::end_block()
{
	if (block_name.empty()) {
		return;
	}
	if (!stack.empty()) {
		dest.define(block_name, stack.back());
		stack.clear();
	} else {
		fail("identifier " + block_name + " defines an empty block");
	}
	symbols.clear();
	block_name.clear();
}

void dfg::reader::decl(std::string text)
{
	// A declaration begins with a name, may have parameters, and marks the
	// end of the previous block and the beginning of a new one.
	end_block();
	size_t pos = 0;
	while (pos < text.size()) {
		char c = text[pos];
		if (isblank(c)) break;
		if (!isgraph(c)) break;
		++pos;
	}
	block_name = text.substr(0, pos);
	while (pos < text.size()) {
		char c = text[pos];
		if (';' == c) return;
		if (!isblank(c)) {
			loc.column = pos;
			fail("unknown token on declaration line");
			return;
		}
		++pos;
	}
}

void dfg::reader::line(std::string text)
{
	// If the line begins with text, it is a top-level declaration.
	// If the line begins with whitespace, it is a statement which continues
	// the current top-level block.
	if (text.empty()) {
		return;
	}
	if (isblank(text.front())) {
		stmt(text);
	} else {
		decl(text);
	}
}

dfg::reader::reader(dfg::unit &_dest, std::istream &src, std::ostream &_err):
	err(_err),
	dest(_dest)
{
	// A serialized DFG is a text file composed of lines containing tokens.
	// A line consists of a sequence of zero or more tokens followed by an
	// optional comment, delimited by ';', which continues to end of line.
	// Tokens are sequences of isgraph() delimited by sequences of isspace().
	// Lines are evaluated top to bottom, tokens are evaluated right to left.
	// We will extract the tokens from this stream and eval() them in order.
	for (std::string text; std::getline(src, text);) {
		++loc.line;
		loc.column = 0;
		line(text);
	}
	end_block();
}


