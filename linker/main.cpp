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
#include "asmjit.h"

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

// OPERATIONS
// arithmetic
//	add num* => num
//  sub num* => num
//  mul num* => num
//  quo num, num => num
//  rem num, num => num
//  shl int, index => int
//  shr int, index => int
// bitwise
//  and int* => int
//  or int* => int
//  xor int* => int
// comparison
//  cmp num* => bool ; =
//  ord num* => bool ; <=
//  seq num* => bool ; <
// branching
//  sel bool, value-true, value-false
// create compound values
//  capture <block>, <value> => closure
//	pack <value> (, value)+ => structure
//  concat <value> => list
// lists
//  first list => value ; first element in the list
//  take index, list => list ; the first N elements of list
//  drop index, list => list ; list without its first N elements
//  last list => value ; last element in the list
//  item index, list => value ; the Nth item in the list
//  length list => index ; number of elements in the list
//  join value* => list


namespace {
class linker
{
	asmjit::JitRuntime runtime;
	asmjit::X86Assembler dest;
	std::map<std::string, asmjit::Label> labels;
	asmjit::Label find(std::string block)
	{
		auto iter = labels.find(block);
		if (iter != labels.end()) {
			return iter->second;
		}
		asmjit::Label l = dest.newLabel();
		labels[block] = l;
		return l;
	}
public:
	linker(): dest(&runtime) {}
	void load(std::string path)
	{
		std::shared_ptr<cpptoml::table> file = cpptoml::parse_file(path);
		for (auto iter: *file) {
			dest.bind(find(iter.first));
			std::shared_ptr<cpptoml::table> block = iter.second->as_table();
			// resolve "in" to symbol table
			// resolve "env" to symbol table
			// evaluate "next"
			// evaluate "out"
			// jump to "next"
		}
	}
};
} // namespace

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

