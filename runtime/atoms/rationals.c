// Copyright 2013 Mars Saxman
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

#include "rationals.h"
#include "booleans.h"
#include "symbols.h"
#include "exceptions.h"
#include "fixints.h"
#include "bigints.h"
#include "numbers.h"
#include "relations.h"
#include "floats.h"
#include <assert.h>
#include "macros.h"

// The rational number is a pair of integers, numerator and denominator, with
// no common factors. The denominator will be positive and greater than 1.
// Should the result of a computation leave us with a number whose denominator
// is 1, we will return an integer instance instead.

#define RATIONAL_SLOT_COUNT 2
#define RATIONAL_NUMERATOR_SLOT 0
#define RATIONAL_DENOMINATOR_SLOT 1

static value_t NotAnInteger( zone_t zone )
{
	return ThrowCStr( zone, "non-integer operand in an integer operation" );
}

#define STANDARD_CHECK(sym) \
	ARGCHECK_2(left, right); \
	if (!IsANumber( right )) return NaNExp( zone, right ); \
	if (IsAFloat( right )) { \
		value_t left_numer = left->slots[RATIONAL_NUMERATOR_SLOT]; \
		value_t left_denom = left->slots[RATIONAL_DENOMINATOR_SLOT]; \
		left = FloatConvertRational( zone, left_numer, left_denom ); \
		return METHOD_1( left, (sym), right ); \
	} while(0)

#define STANDARD_CRACK \
	value_t left_numer = left->slots[RATIONAL_NUMERATOR_SLOT]; \
	value_t left_denom = left->slots[RATIONAL_DENOMINATOR_SLOT]; \
	value_t right_numer = IsAnInteger( right ) ? right : \
			right->slots[RATIONAL_NUMERATOR_SLOT]; \
	value_t right_denom = IsAnInteger( right ) ? num_one : \
			right->slots[RATIONAL_DENOMINATOR_SLOT]

static value_t Rational_function( PREFUNC, value_t selector );

static value_t Rational_compare_to( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_compare_to); STANDARD_CRACK;
	// Create a common denominator for these fractions. Compare numerators.
	left_numer = METHOD_1( left_numer, sym_multiply, right_denom );
	right_numer = METHOD_1( right_numer, sym_multiply, left_denom );
	return METHOD_1( left_numer, sym_compare_to, right_numer );
}

static value_t Rational_add( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_add); STANDARD_CRACK;
	// Create a common denominator for these fractions. Add the numerators.
	left_numer = METHOD_1( left_numer, sym_multiply, right_denom );
	right_numer = METHOD_1( right_numer, sym_multiply, left_denom );
	value_t out_denom = METHOD_1( left_denom, sym_multiply, right_denom );
	value_t out_numer = METHOD_1( left_numer, sym_add, right_numer );
	return RationalFromIntegers( zone, out_numer, out_denom );
}

static value_t Rational_subtract( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_subtract); STANDARD_CRACK;
	// Create a common denominator for these fractions. Subtract numerators.
	left_numer = METHOD_1( left_numer, sym_multiply, right_denom );
	right_numer = METHOD_1( right_numer, sym_multiply, left_denom );
	value_t out_denom = METHOD_1( left_denom, sym_multiply, right_denom );
	value_t out_numer = METHOD_1( left_numer, sym_subtract, right_numer );
	return RationalFromIntegers( zone, out_numer, out_denom );
}

static value_t Rational_multiply( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_multiply); STANDARD_CRACK;
	// Multiply numerators and denominators.
	value_t out_numer = METHOD_1( left_numer, sym_multiply, right_numer );
	value_t out_denom = METHOD_1( left_denom, sym_multiply, right_denom );
	return RationalFromIntegers( zone, out_numer, out_denom );
}

static value_t Rational_divide( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_divide); STANDARD_CRACK;
	// Multiply left numerator by right denominator and vice versa.
	value_t out_numer = METHOD_1( left_numer, sym_multiply, right_denom );
	value_t out_denom = METHOD_1( left_denom, sym_multiply, right_numer );
	return RationalFromIntegers( zone, out_numer, out_denom );
}

static value_t Rational_modulus( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_modulus);
	return ThrowCStr( zone, "not yet implemented" );
}

static value_t Rational_exponentiate( PREFUNC, value_t left, value_t right )
{
	STANDARD_CHECK(sym_exponentiate); STANDARD_CRACK;
	if (IsAFixint( right_numer ) &&
			IsAFixint( right_denom ) &&
			1 == IntFromFixint( right_denom )) {
		// We can raise a rational to an integer exponent.
		int exponent = IntFromFixint( right_numer );
		bool flip = exponent < 1;
		if (flip) {
			exponent = -exponent;
		}
		value_t val = num_one;
		while (exponent-- > 0) {
			val = METHOD_1( val, sym_multiply, left );
		}
		if (flip) {
			val = METHOD_1( num_one, sym_divide, val );
		}
		return val;
	}
	// If the exponent is a fraction, the result probably isn't going to be
	// rational, so we'll just shift up to floats now and convert as a real.
	// It would be nice to detect perfect squares/cubes/etc and return a
	// rational when we can, but this will work for now.
	left = FloatConvertRational( zone, left_numer, left_denom );
	return METHOD_1( left, sym_exponentiate, right );
}

static value_t Rational_numerator( PREFUNC, value_t val )
{
	ARGCHECK_1( val );
	return val->slots[RATIONAL_NUMERATOR_SLOT];
}

static value_t Rational_denominator( PREFUNC, value_t val )
{
	ARGCHECK_1( val );
	return val->slots[RATIONAL_DENOMINATOR_SLOT];
}

static value_t Rational_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD( compare_to, Rational_compare_to )
	DEFINE_METHOD( add, Rational_add )
	DEFINE_METHOD( subtract, Rational_subtract )
	DEFINE_METHOD( multiply, Rational_multiply )
	DEFINE_METHOD( divide, Rational_divide )
	DEFINE_METHOD( modulus, Rational_modulus )
	DEFINE_METHOD( exponentiate, Rational_exponentiate )
	DEFINE_METHOD( numerator, Rational_numerator )
	DEFINE_METHOD( denominator, Rational_denominator )
	if (selector == sym_is_number) return &True_returner;
	if (selector == sym_is_rational) return &True_returner;
	if (selector == sym_is_integer) return &False_returner;
	return ThrowMemberNotFound( zone, selector );
}

bool IsARational( value_t exp )
{
	return exp && exp->function == (function_t)Rational_function;
}

static int CompareToZero( zone_t zone, value_t num )
{
	value_t relation = METHOD_1( num_zero, sym_compare_to, num );
	return IntFromRelation( zone, relation );
}

static value_t GCD( zone_t zone, value_t n, value_t m )
{
	// Good old Euclid's algorithm. Find the greatest common denominator of
	// these two numbers. We assume that 'm' is positive, because we know that
	// RationalFromIntegers() always makes the denominator positive. We don't
	// know whether the numerator is positive, so we'll flip it if necessary.
	if (CompareToZero( zone, n ) < 0) {
		n = METHOD_1( num_zero, sym_subtract, n );
	}
	// Take the remainder until we have no remainder left.
	while (0 != CompareToZero( zone, n ) ) {
		value_t remainder = METHOD_1( m, sym_modulus, n );
		m = n;
		n = remainder;
	}
	return m;
}

static value_t IntegerQuotient( zone_t zone, value_t numer, value_t denom )
{
	// Division yielding quotient and ignoring remainder. We will leave the
	// detection of div by zero errors to the bigint and fixint functions,
	// because they can do a quick test without calling the comparison method.
	if (IsAnException( numer )) return numer;
	if (IsAnException( denom )) return denom;
	if (IsAFixint( numer ) && IsAFixint( denom )) {
		return FixintQuotient( zone, numer, denom );
	} else if (IsAnInteger( numer ) && IsAnInteger( denom )) {
		return BigintQuotient( zone, numer, denom );
	} else if (IsANumber( numer ) && IsANumber( denom )) {
		return NotAnInteger( zone );
	} else {
		return NaNExp( zone, numer );
	}
}

value_t RationalFromIntegers( zone_t zone, value_t numer, value_t denom )
{
	// Create a rational object by dividing these integers.
	assert( IsAnInteger( numer ) );
	assert( IsAnInteger( denom ) );

	// Check the sign of the denominator. We prefer positive denominators, so
	// if it is negative we will flip signs in order to put the sign on the
	// numerator. This also cancels signs if both are negative.
	int rel = CompareToZero( zone, denom );
	if (rel < 0) {
		numer = METHOD_1( num_zero, sym_subtract, numer );
		denom = METHOD_1( num_zero, sym_subtract, denom );
	}
	else if (rel == 0) {
		return DivByZeroExp( zone );
	}

	// Reduce the fraction by eliminating common factors.
	value_t divisor = GCD( zone, numer, denom );
	numer = IntegerQuotient( zone, numer, divisor );
	denom = IntegerQuotient( zone, denom, divisor );

	// If we have reduced the denominator all the way down to one, return the
	// numerator - we have returned to the land of the integers.
	value_t relation = METHOD_1( num_one, sym_compare_to, denom );
	if (0 == IntFromRelation( zone, relation )) {
		return numer;
	}

	// Package these two integers into our fraction object for output.
	struct closure *out = ALLOC( Rational_function, RATIONAL_SLOT_COUNT );
	out->slots[RATIONAL_NUMERATOR_SLOT] = numer;
	out->slots[RATIONAL_DENOMINATOR_SLOT] = denom;
	return out;
}

value_t RationalNumerator( value_t rat )
{
    assert( IsARational( rat ) );
    return rat->slots[RATIONAL_NUMERATOR_SLOT];
}

value_t RationalDenominator( value_t rat )
{
    assert( IsARational( rat ) );
    return rat->slots[RATIONAL_DENOMINATOR_SLOT];
}

