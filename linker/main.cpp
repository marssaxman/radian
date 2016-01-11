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

#include <iostream>
#include <fstream>
#include "assembler.h"

// TYPE SYSTEM
// for easy indexing and comparison, we will identify types with strings
//	byte, 'b', uint8_t
//	index, 'u', uint32_t
//	fixed, 'i', int64_t
//	float, 'f', double
//	array, '[' + element + ']', struct { element *head; element *tail; }
//	closure, '(' + param + ')' struct { void *env; void(*proc)(param); }
// structures concatenate a series of fields (using natural alignment),
// so their type strings are the concatenation of their field type strings

// ABI
// each block invocation gets a parameter and an environment value
// parameters are provided in registers, according to type:
//  byte: AL
//  index: EAX
//  fixed: EAX, ECX [low, high]
//  float: ST0
//  array: EAX:ECX [head, tail]
//  closure: EAX:ECX [env, proc]
//  structure: EAX (passed by address)
// environment is passed by address in EBP
// the values of other registers are undefined

// MEMORY
// allocate memory by decrementing ESP using some even multiple of 4
// deallocate before jumping to the next block, depending on parameter type:
// closure, struct, array:
//	ESP = min(EBP, EAX & ~3)
// any other type:
//  ESP = EBP

namespace {
class DFG
{
	struct {
		std::string path;
		size_t line = 0;
		size_t column = 0;
		bool valid = true;
	} parse;
	std::ostream &fail();
	void read_op(std::string instruction);
	void read_term(char prefix, std::string body);
	void read_token(std::string token);
	void read_line(std::string text);
public:
	void read(std::istream &src);
	void load(std::string path);
	bool valid() const { return parse.valid; }
};
}

std::ostream &DFG::fail()
{
	parse.valid = false;
	std::cerr << parse.path << ":" << std::dec;
	std::cerr << parse.line << ":" << parse.column << ": ";
	return std::cerr;
}

void DFG::read_op(std::string op)
{
	fail() << "undefined operation \"" << op << "\"" << std::endl;
}

void DFG::read_term(char prefix, std::string body)
{
	switch (prefix) {
	default:
		fail() << "undefined prefix \'" << prefix << "\'" << std::endl;
	}
}

void DFG::read_token(std::string token)
{
	std::cout << token << " ";
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

void DFG::read(std::istream &src)
{
	// A serialized DFG is a text file composed of lines containing tokens.
	// A line consists of a sequence of zero or more tokens followed by an
	// optional comment, delimited by ';', which continues to end of line.
	// Tokens are sequences of isgraph() delimited by sequences of isspace().
	// Lines are evaluated top to bottom, tokens are evaluated right to left.
	// We will extract the tokens from this stream and eval() them in order.
	parse.line = 1;
	for (std::string line; std::getline(src, line); ++parse.line) {
		read_line(line);
	}
}

void DFG::load(std::string path)
{
	parse.path = path;
	std::ifstream file(path);
	read(file);
}

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	if (argc <= 1) {
		std::cerr << "radian-link: fail: no input files" << std::endl;
		return EXIT_FAILURE;
	}
	std::cerr << std::showbase;
	DFG dfg;
	bool valid = true;
	for (int i = 1; i < argc; ++i) {
		dfg.load(argv[i]);
		valid &= dfg.valid();
	}
	return valid? EXIT_SUCCESS: EXIT_FAILURE;
}

