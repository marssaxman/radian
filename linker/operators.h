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

UNARY(jump)		// block: execute a fully bound closure and return its result
UNARY(notl)		// bool -> bool:logical inversion
				// int -> int: bitwise inversion
UNARY(test)		// int -> bool: true if not zero, false if equal to zero
UNARY(null)		// array -> bool: true if array contains no elements
UNARY(peek)		// array -> value: value of first array element
UNARY(next)		// array -> array: array containing all but the first element
BINARY(call)	// block, val: transfer control and supply an argument value
BINARY(bind)	// block, val -> closure: bind arg to block, creating a closure
BINARY(diff)	// int, int -> int: arithmetic difference
BINARY(xorl)	// bool, bool -> bool: true if exactly one input is true
				// int, int -> int: exclusive or on each bit in the word
BINARY(item)	// array, int -> value: get N'th array element value
BINARY(head)	// array, int -> array: first N elements
BINARY(skip)	// array, int -> array: all but the first N elements
BINARY(tail)	// array, int -> array: last N elements
BINARY(drop)	// array, int -> array: all but the last N elements
TERNARY(bcc)	// bool, closure, closure: if the condition is true, jump to
				// the first closure, otherwise jump to the second one.
TERNARY(sel)	// bool, value, value: return the first value if the condition
				// is true, otherwise return the second value.
TERNARY(loop)	// bool, value, block: if the condition is true, reinvoke the
				// current block with a new argument value; otherwise, call
				// the target function with that argument value.
VARIADIC(sum)	// int* -> int: add up all the values
VARIADIC(andl)	// bool* -> bool: true if all values are true
				// int* -> int: each bit set if it is set in all inputs
VARIADIC(orl)	// bool* -> bool: true if any value is true
				// int* -> int: each bit set if it is set in any input
VARIADIC(cpeq)	// int* -> bool: true if all values are equal
VARIADIC(cpge)	// int* -> bool: true if values are equal or ascending
VARIADIC(cpgt)	// int* -> bool: true if values are non-equal and ascending
VARIADIC(tuple)	// value* -> tuple: create a structure from these values
VARIADIC(array)	// value* -> array: create an array from equal-typed values
VARIADIC(cat)	// array* -> array: join the contents of these arrays

#undef UNARY
#undef BINARY
#undef TERNARY
#undef VARIADIC
#undef OPERATOR

