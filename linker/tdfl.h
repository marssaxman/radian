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

#ifndef TDFL_H
#define TDFL_H

#include <string>
#include <vector>
#include <iostream>
#include "dfg.h"

namespace tdfl { // test data flow language

// instruction names:
//   unary: notl, test, null, peek, next
//   binary: diff, xorl, item, head, skip, tail, drop
//   ternary: cond
//   variadic: sum, all, any, equ, ord, asc, tuple, array, cat
// operand syntax:
//   $digits - integer literal (unsigned, hex)
//	 _name - link reference to another block
//	 %name - value of a symbol previously defined in this block
//	 * - parameter value for this function invocation
//	 *digits - value of numbered field in parameter tuple
//	 ^ - value produced by the previous instruction
//   @digits(operand) - value of numbered field in operand tuple

struct instruction
{
	std::string def;
	std::string op;
	std::vector<std::string> args;
};
typedef std::vector<instruction> block;
dfg::block &&build(const block&, std::ostream &log);

}

#endif //TDFL_H
