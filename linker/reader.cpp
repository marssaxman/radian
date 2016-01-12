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

const dfg::node *dfg::reader::top()
{
	if (stack.empty()) {
		fail("stack underflow");
		push(current->add<dummy>());
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
	push(current->add<dfg::literal>(value));
}

void dfg::reader::symbol(std::string name)
{
	auto iter = symbols.find(name);
	const dfg::node *out = nullptr;
	if (iter != symbols.end()) {
		out = iter->second;
	} else {
		fail("undefined symbol \"" + name + "\"");
		symbols[name] = out = current->add<dummy>();
	}
	push(out);
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
		fail("undefined operation \"" + text + "\"");
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
	// A line may have a definition, a body, and/or a comment.
	// If the first token on the line ends with a colon, that token is a
	// symbol definition.
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
			return;
		}
		symbols[defsym] = top();
	}
}

dfg::reader::reader(dfg::unit &_dest, std::istream &src, std::ostream &_err):
	err(_err),
	dest(_dest),
	current(_dest.add())
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


