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
#include "reader.h"
#include "logstream.h"

// NAME MANGLING
// one char for each atom, delimiter pairs for compound types:
//  b = byte, s = short, w = word, l = long
//  f = float, d = double
// a structure simply concatenates its field types
// a nested structure is wrapped in '{' and '}'
// an array wraps its element type in '[' and ']'
// a closure wraps its parameter type in '(' and ')' 

// MEMORY LAYOUT
// structure fields use natural alignment in order
// arrays are a struct describing a range of elements:
// template<typename T> struct array<T>{ T *head; T *tail; };
// a closure combines a proc ptr and an environment value:
//	struct { void *env; void(*proc)(param); }

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

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	logstream log(std::string(argv[0]) + ": ", std::cerr);
	if (argc <= 1) {
		log << "no input files" << std::endl;
	}
	dfg::unit data;
	for (int i = 1; i < argc; ++i) {
		std::string path(argv[i]);
		std::ifstream src(path);
		logstream err(path + ":", log);
		dfg::reader(data, src, err);
	}
	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

