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
#include "buffer.h"
#include "symbols.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "booleans.h"
#include "exceptions.h"
#include "strings.h"
#include "relations.h"
#include "rationals.h"
#include "floats.h"
#include "macros.h"
#include "bigints.h"
#include <limits.h>


static value_t NotAnInteger( zone_t zone )
{
	return ThrowCStr( zone, "non-integer operand in an integer operation" );
}

static value_t Fixint_Compare_to( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return RelationFromInt( lval - rval );
	} else if (IsABigint( right )) {
		// Let the bigint do all the work. It knows how to deal with fixints.
		return InvertRelation( zone, METHOD_1( right, sym_compare_to, left ) );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		return METHOD_1( left, sym_compare_to, numer );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_compare_to, right );
	}
	else return NaNExp( zone, right );
}

static value_t Fixint_Add( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		if (rval > 0 ? (lval > INT_MAX - rval) : (lval < INT_MIN - rval)) {
			// We are going to overflow. Promote lval to bigint.
			left = MakeTemporaryBigint( zone, lval );
			return METHOD_1( left, sym_add, right );
		}
		return NumberFromInt( zone, lval + rval );
	} else if (IsABigint( right )) {
		// yay for commutativity; make the bigint class handle it
		return METHOD_1( right, sym_add, left );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		left = METHOD_1( left, sym_add, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_add, right );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Subtract( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		if (rval > 0 ? (lval < INT_MIN + rval) : (lval > INT_MAX + rval)) {
			// We are going to overflow. Switch up to bigint land.
			left = MakeTemporaryBigint( zone, lval );
			return METHOD_1( left, sym_subtract, right );
		}
		return NumberFromInt( zone, lval - rval );
	} else if (IsABigint( right )) {
		left = MakeTemporaryBigint( zone, *BUFDATA(left, int) );
		return METHOD_1( left, sym_subtract, right );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		left = METHOD_1( left, sym_subtract, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_subtract, right );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Multiply( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		if (rval > 0 ? lval > INT_MAX/rval
				|| lval < INT_MIN/rval
				: (rval < -1 ? lval > INT_MIN/rval
				|| lval < INT_MAX/rval
				: rval == -1
				&& lval == INT_MIN) ) {
			left = MakeTemporaryBigint( zone, lval );
			return METHOD_1( left, sym_multiply, right );
		}
		return NumberFromInt( zone, lval * rval );
	} else if (IsABigint( right )) {
		// multiplication is commutative
		return METHOD_1( right, sym_multiply, left );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_multiply, right );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Divide( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		return RationalFromIntegers( zone, left, right );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		return RationalFromIntegers( zone, left, numer );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_divide, right );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Modulus( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval % rval );
	} else if (IsABigint( right )) {
		// Every bigint has a greater magnitude than every fixint, so there can
		// never be any remainder when the bigint is the denominator.
		return num_zero;
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Exponentiate( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int exponent = *BUFDATA( right, int );
		bool flip = exponent < 1;
		if (flip) {
			exponent = -exponent;
		}
		value_t out = num_one;
		while (exponent-- > 0) {
			out = METHOD_1( out, sym_multiply, left );
		}
		if (flip) {
			out = METHOD_1( num_one, sym_divide, out );
		}
		return out;
	} else {
		left = NumberFromDouble( zone, *BUFDATA( left, int ) );
		return METHOD_1( left, sym_exponentiate, right );
	}
	return ThrowCStr( zone, "unimplemented" );
}

static value_t Fixint_ShiftLeft( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval << rval );
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_ShiftRight( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval >> rval );
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_BitAnd( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval & rval );
	} else if (IsABigint( right )) {
		return METHOD_1( right, sym_bit_and, left );
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_BitOr( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval | rval );
	} else if (IsABigint( right )) {
		return METHOD_1( right, sym_bit_or, left );
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_BitXor( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int lval = *BUFDATA( left, int );
		int rval = *BUFDATA( right, int );
		return NumberFromInt( zone, lval ^ rval );
	} else if (IsABigint( right )) {
		return METHOD_1( right, sym_bit_xor, left );
	} else if (IsANumber( right )) {
		return NotAnInteger( zone );
	} else return NaNExp( zone, right );
}

static value_t Fixint_Numerator( PREFUNC, value_t number )
{
	ARGCHECK_1( number );
	// integers are their own numerators, by definition
	return number;
}

static value_t Fixint_Denominator( PREFUNC, value_t number )
{
	ARGCHECK_1( number );
	// integers are always denominated by 1, by definition
	return num_one;
}

static value_t Fixint_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(compare_to, Fixint_Compare_to)
	DEFINE_METHOD(add, Fixint_Add)
	DEFINE_METHOD(subtract, Fixint_Subtract)
	DEFINE_METHOD(multiply, Fixint_Multiply)
	DEFINE_METHOD(divide, Fixint_Divide)
	DEFINE_METHOD(modulus, Fixint_Modulus)
	DEFINE_METHOD(exponentiate, Fixint_Exponentiate)
	DEFINE_METHOD(shift_left, Fixint_ShiftLeft)
	DEFINE_METHOD(shift_right, Fixint_ShiftRight)
	DEFINE_METHOD(bit_and, Fixint_BitAnd)
	DEFINE_METHOD(bit_or, Fixint_BitOr)
	DEFINE_METHOD(bit_xor, Fixint_BitXor)
	DEFINE_METHOD(numerator, Fixint_Numerator)
	DEFINE_METHOD(denominator, Fixint_Denominator)
	if (selector == sym_is_number) return &True_returner;
	if (selector == sym_is_rational) return &True_returner;
	if (selector == sym_is_integer) return &True_returner;
	return ThrowMemberNotFound( zone, selector );
}


bool IsAFixint( value_t obj )
{
	return obj && obj->function == (function_t)Fixint_function;
}

// NumberFromInt
//
// Internal factory for use by runtime code that needs number objects. Give it
// an integer and it will create an integer object out of it.
//
value_t NumberFromInt( zone_t zone, int data )
{
	return (value_t)clone_buffer(
			zone,
			(function_t)Fixint_function,
			sizeof(int),
			&data );
}

// IntFromFixint
//
// Internal accessor used to deconstruct a number object as an integer.
// Obviously won't accept a rational; you must truncate it first.
//
int IntFromFixint( value_t data )
{
	assert( IsAFixint( data ) );
	return *BUFDATA(data, int);
}

// FixintQuotient
//
// Perform a straight-up truncating division on this pair of integers,
// returning the quotient and ignoring the remainder.
//
value_t FixintQuotient( zone_t zone, value_t left, value_t right )
{
	assert( IsAFixint( left ) && IsAFixint( right ) );
	int lval = *BUFDATA( left, int );
	int rval = *BUFDATA( right, int );
	if (0 == rval) return DivByZeroExp( zone );
	return NumberFromInt( zone, lval / rval );
}

