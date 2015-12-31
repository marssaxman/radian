// Copyright 2009-2013 Mars Saxman
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software in a
// product, an acknowledgment in the product documentation would be appreciated
// but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.


#include "numbers.h"
#include "fixints.h"
#include "bigints.h"
#include "rationals.h"
#include "floats.h"
#include <assert.h>
#include "macros.h"
#include "symbols.h"

value_t num_zero;
value_t num_one;
value_t num_infinity;

// Tower style architecture: all rationals are numbers, all integers are
// rational.
// - Methods available on the notional "number" interface:
//     is_number, is_rational, is_integer
//     add subtract multiply divide modulus exponentiate compare_to
// - Methods available on the "rational" interface, a superset of "number":
//     numerator denominator
// - Methods available on the "integer" interface, a superset of "rational":
//     shift_left shift_right bit_and bit_or bit_xor
// We have four number representations:
//	fixint is a buffer containing one int
//	bigint is an array of digit_t, which is unsigned, with low bit as sign
//	rational is a closure with a numerator and a denominator, which are ints
//	float is a buffer containing one double

// The internal IsA functions describe object implementations and do not refer
// to the types in the numeric tower. 
bool IsANumber( value_t obj )
{
	return IsAnInteger( obj ) || IsARational( obj ) || IsAFloat( obj );
}

bool IsAnInteger( value_t obj )
{
    return IsAFixint( obj ) || IsABigint( obj );
}

static value_t BigLiteral(
		zone_t zone,
		unsigned numer_int,
		unsigned denom_int,
		bool fractional,
		const char *data,
		size_t length )
{
	value_t numer = NumberFromInt( zone, numer_int );
	value_t denom = NumberFromInt( zone, denom_int );
	value_t ten = NumberFromInt( zone, 10 );
	while (length--) {
		char ch = *data++;
		if ('.' == ch) {
			fractional = true;
			continue;
		}
		assert( ch >= '0' && ch <= '9' );
		numer = METHOD_1( numer, sym_multiply, ten );
		numer = METHOD_1( numer, sym_add, NumberFromInt( zone, ch - '0' ) );
		if (fractional) {
			denom = METHOD_1( denom, sym_multiply, ten );
		}
	}
	return fractional ? RationalFromIntegers( zone, numer, denom ) : numer;
}

// NumberLiteral
//
// Direct compiler-generated entrypoint for number literals. We get an ASCII
// string representation of the number, in decimal. If it contains a decimal
// point, it is a rational number; otherwise, an integer. It might be nicer to
// have the compiler render this into binary first and pass it in as a blob.
// It would also be nice to execute all of the literals at startup and just
// refer to the objects later, because otherwise we have to re-execute this on
// every trip through the function which uses the literal, and that's a waste.
//
value_t NumberLiteral( zone_t zone, const char *data, size_t length )
{
	assert( data && length );
	// Let's try it the inexpensive way first, using simple integer math and
	// producing a fixint or a ratio of fixints. If we leave the range of a
	// fixint, we'll have to switch up to bigints, but that's expensive and we
	// don't usually need to go there.
	unsigned numer = 0;
	unsigned denom = 1;
	// This is the largest number we can reach while being certain that we will
	// still be able to accommodate the next digit. If we begin an iteration
	// with a number larger than this, we may overflow, and must switch up to
	// a bigint representation. We assume unsigned will be either 4 or 8 bytes.
	unsigned limit = ((~(unsigned)0) >> 1) / 10 - 1;
	bool fractional = false;
	while (length--) {
		if (numer > limit) {
			// We might be about to overflow. Go do it the expensive way.
			return BigLiteral( zone, numer, denom, fractional, data, ++length );
		}
		char ch = *data++;
		if ('.' == ch) {
			fractional = true;
			continue;
		}
		assert( ch >= '0' && ch <= '9' );
		numer *= 10;
		numer += ch - '0';
		if (fractional) {
			denom *= 10;
		}
	}
	value_t out = NumberFromInt( zone, numer );
	if (fractional) {
		value_t denom_obj = NumberFromInt( zone, denom );
		out = RationalFromIntegers( zone, out, denom_obj );
	}
	return out;
}

value_t NaNExp( zone_t zone, value_t obj )
{
	// this thing is not a number but was used in a numeric context. this is
	// bad. report an error.
	return ThrowCStr( zone, "non-numeric operand in a numeric operation" );
}

value_t DivByZeroExp( zone_t zone )
{
	return ThrowCStr( zone, "division by zero" );
}

// init_numbers
//
// Allocate some common numbers the runtime uses frequently. This is supposed
// to be easier than calling NumberFromInt all the time. It would be nice not
// to need NumberFromInt very often.
//
void init_numbers(zone_t zone)
{
	num_zero = NumberFromInt( zone, 0 );
	num_one = NumberFromInt( zone, 1 );
	init_bigints( zone );
}

#if RUN_TESTS
void test_numbers( zone_t zone )
{
    test_bigints( zone );
}
#endif
