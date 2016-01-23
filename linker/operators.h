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


// Macro system for generating instruction boilerplate.
// Define macros UNARY, BINARY, TERNARY, and VARIADIC before including this
// file, then do something useful with the names provided. This header will
// #undef the macros after it's done in case you want to include it more than
// once in a single translation unit.

#ifndef OPERATOR
#define OPERATOR(x)
#endif
#ifndef UNARY
#define UNARY(x) OPERATOR(x)
#endif
#ifndef BINARY
#define BINARY(x) OPERATOR(x)
#endif
#ifndef TERNARY
#define TERNARY(x) OPERATOR(x)
#endif
#ifndef VARIADIC
#define VARIADIC(x) OPERATOR(x)
#endif

// boolean logic
UNARY(test)		// 0 if 0, otherwise 1
UNARY(notl)		// 1 if 0, otherwise 0
BINARY(xorl)	// 0 if both values are zero or nonzero, otherwise 1
VARIADIC(orl)	// 1 if any operand is nonzero, otherwise 0
VARIADIC(andl)	// 1 if all operands are nonzero, otherwise 0

// basic integer arithmetic
UNARY(notw)		// invert all bits
VARIADIC(orw)	// bitwise or all operand values together
VARIADIC(andw)	// bitwise and all operand values together
BINARY(xorw)	// exclusive or of all bits
VARIADIC(addw)	// add all values together
BINARY(subw)	// difference between the two values

// numeric comparisons
VARIADIC(cpeq)	// true if all values are the same
VARIADIC(cpge)	// true if each value is greater than or equal to the previous
VARIADIC(cpgt)	// true if each value is greater than the previous one

// functions and control flow
BINARY(jump)	// closure, value: invoke closure with argument value
BINARY(bind)	// closure, value: partial application, creates closure
TERNARY(sel)	// bool, tval, fval: return tval if true, fval if false

// data structures
VARIADIC(tuple)	// value* -> tuple: fixed group of heterogeneous values
VARIADIC(array) // value* -> array: variable-length group of like values
VARIADIC(join)	// struct* -> struct: join elements into a larger structure
TERNARY(slice)	// struct, begin, end: extract elements as a smaller struct

// array-specific operations
UNARY(empty) 	// array -> bool: true if array contains no elements
UNARY(peek)		// array -> value: value of first array element
UNARY(next)		// array -> array: array containing all but the first element
BINARY(item)	// array, int -> value: get N'th array element value
BINARY(head)	// array, int -> array: first N elements
BINARY(skip)	// array, int -> array: all but the first N elements
BINARY(tail)	// array, int -> array: last N elements
BINARY(drop)	// array, int -> array: all but the last N elements

#undef UNARY
#undef BINARY
#undef TERNARY
#undef VARIADIC
#undef OPERATOR

