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

