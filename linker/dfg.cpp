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

void dfg::reader::fail(std::string msg)
{
	using namespace std;
	err << dec << pos.line << ":" << pos.column << ": " << msg << endl;
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
	current.nodes.emplace_back(new dfg::literal(value));
	stack = current.nodes.size();
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
			default: fail("unknown token prefix '" + std::string(1, c) + "'");
		}
	} else {
		fail("undefined operation \"" + text + "\"");
	}
}

void dfg::reader::line(std::string line)
{
	// Truncate the line at the comment, if present.
	size_t end = line.size();
	for (size_t i = 0; i < end; ++i) {
		if (';' == line[i]) {
			end = i;
		}
	}
	while (end > 0) {
		// Find the length of the token character sequence, if present.
		size_t start = end;
		do {
			char c = line[start - 1];
			if (isblank(c) || !isgraph(c)) break;
			--start;
		} while (start > 0);
		if (start < end) {
			// If we found a token, go process it.
			pos.column = start;
			token(line.substr(start, end - start));
			end = start;
		} else {
			// If what we found was not a token, skip it.
			--end;
		}
	}
}

dfg::reader::reader(dfg &_dest, std::istream &src, std::ostream &_err):
	dest(_dest),
	err(_err)
{
	// A serialized DFG is a text file composed of lines containing tokens.
	// A line consists of a sequence of zero or more tokens followed by an
	// optional comment, delimited by ';', which continues to end of line.
	// Tokens are sequences of isgraph() delimited by sequences of isspace().
	// Lines are evaluated top to bottom, tokens are evaluated right to left.
	// We will extract the tokens from this stream and eval() them in order.
	pos.line = 1;
	for (std::string text; std::getline(src, text); ++pos.line) {
		line(text);
	}
}

void dfg::read(std::istream &src, std::ostream &err)
{
	reader(*this, src, err);
}

