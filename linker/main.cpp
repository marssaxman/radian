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
#include "tdfl-build.h"
#include "tdfl-print.h"

int main(int argc, const char *argv[])
{
	// Read in an array of function DFGs, link them together, and write them
	// out as a bootable kernel image.
	logstream log(std::string(argv[0]) + ": ", std::cerr);

	tdfl::code test = {
		"_add1.ii:",
		"a:		0",
		"b:		1",
		"		sum %a, %b",
		"_add2.ii:",
		"		cpeq 0, $0",
		"		sel ^, _done, _recurse",
		"		call ^, *",
		"_recurse.ii:",
		"tmp2:	diff 0, $1",
		"tmp3:	diff 1, $1",
		"tmp4:	tuple %tmp2, %tmp3",
		"		call _add2.ii, %tmp4",
		"_done.ii: 1",
	};

	dfg::unit code = tdfl::build(test, log);
	tdfl::print(code, std::cout);

	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

