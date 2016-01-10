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
#include <cpptoml.h>
#include "assembler.h"
#include "emit.h"

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
class linker
{
	assembler dest;
	void parse_file(std::shared_ptr<cpptoml::table>);
	void parse_block(std::shared_ptr<cpptoml::table>);
public:
	void load(std::string path);
};
}

void linker::load(std::string path)
{
	std::shared_ptr<cpptoml::table> file = cpptoml::parse_file(path);
	parse_file(file);
}

void linker::parse_file(std::shared_ptr<cpptoml::table> file)
{
	for (auto iter: *file) {
		dest.bind(iter.first);
		parse_block(iter.second->as_table());
	}
}

void linker::parse_block(std::shared_ptr<cpptoml::table> block)
{
	// the block table has five members:
	//	in - type of the parameter value
	//	env - type of the environment value
	//  exp - array of expression nodes
	//	out - expression to pass along
	//	next - continuation to invoke
	emit::block d;
	d.in_type = block->get_as<std::string>("in").value_or("");
	d.env_type = block->get_as<std::string>("env").value_or("");
	d.out_exp = *block->get_as<int64_t>("out");
	d.next_exp = *block->get_as<int64_t>("next");
	for (auto exp: *block->get("exp")->as_array()) {
	}
	emit(d, dest);
}

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	if (argc <= 1) {
		std::cerr << "radian-link: fail: no input files" << std::endl;
		return EXIT_FAILURE;
	}
	linker dest;
	for (int i = 1; i < argc; ++i) {
		try {
			dest.load(argv[i]);
		} catch (const cpptoml::parse_exception &e) {
			std::cerr << "failed to parse " << argv[i];
			std::cerr << ": " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

