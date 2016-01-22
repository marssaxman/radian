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
#include "logstream.h"
#include "tdfl.h"

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	logstream log(std::string(argv[0]) + ": ", std::cerr);

	tdfl::code test = {
        "              sum 0, 1",
		"valid:        cpge ^, $0A",
		"              andl %valid, $1",
	};
	dfg::block code = tdfl::build(test, log);
	tdfl::print(code, std::cout);

	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

