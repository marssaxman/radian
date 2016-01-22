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
//   ternary: sel
//   variadic: sum, andl, orl, cpeq, cpge, cpgt, tuple, array, cat
// operand syntax:
//   $digits - integer literal (hexadecimal)
//	 _name - link reference to another block
//	 %name - value of a symbol previously defined in this block
//	 * - parameter value for this function invocation
//	 ^ - value produced by the previous instruction
//	 number['(' operand ')'] - Nth field of tuple; parameter is default value
// instruction syntax:
// [name':'] instruction [operand [',' operand]*] ['#' comment]

typedef std::vector<std::string> code;
dfg::block build(const code &input, std::ostream &log);
void print(const dfg::block&, std::ostream&);

}

#endif //TDFL_H