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

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	if (argc <= 1) {
		std::cerr << "radian-link: fail: no input files" << std::endl;
		return EXIT_FAILURE;
	}
	asmjit::JitRuntime runtime; // wrong, but it'll do to start out with
	asmjit::X86Assembler dest(&runtime);
	for (int i = 1; i < argc; ++i) {
		using namespace std;
		using namespace cpptoml;
		std::string path = argv[i];
		try {
			shared_ptr<table> blocks = parse_file(path);
			assert(blocks->is_table());
			for (auto iter: *blocks) {
				// name = iter->first
				// body = iter->second
			}
		} catch (const parse_exception &e) {
			cerr << "failed to parse " << path << ": " << e.what() << endl;
			return EXIT_FAILURE;;
		}
	}
	return EXIT_SUCCESS;
}

