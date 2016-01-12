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

std::ostream &DFG::fail()
{
	return log << std::dec << parse.line << ":" << parse.column << ": ";
}

void DFG::read_op(std::string op)
{
	// operation categories:
	// source - push 1
	// sink - pop 1
	// unary - pop 1, push 1
	// binary - pop 2, push 1
	// ternary - pop 3, push 1
	// variable - pop 1 + N, push 1
	fail() << "undefined operation \"" << op << "\"" << std::endl;
}

void DFG::read_term(char prefix, std::string body)
{
	switch (prefix) {
	case '@': // block
	case '%': // symbol
	case '$': // literal
	case '=': // type
	default:
		fail() << "undefined prefix \'" << prefix << "\'" << std::endl;
	}
}

void DFG::read_line(std::string line)
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
			char c = line[start];
			if (ispunct(c)) {
				read_term(c, line.substr(start + 1, end - start - 1));
			} else {
				read_op(line.substr(start, end - start));
			}
			end = start;
		} else {
			// If what we found was not a token, skip it.
			--end;
		}
	}
}

DFG::DFG(std::istream &src, std::ostream &log):
	log(log)
{
	// A serialized DFG is a text file composed of lines containing tokens.
	// A line consists of a sequence of zero or more tokens followed by an
	// optional comment, delimited by ';', which continues to end of line.
	// Tokens are sequences of isgraph() delimited by sequences of isspace().
	// Lines are evaluated top to bottom, tokens are evaluated right to left.
	// We will extract the tokens from this stream and eval() them in order.
	int index = 1;
	for (std::string line; std::getline(src, line); ++index) {
		read_line(line);
	}
}


