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

#include "macros.h"
#include "mathlib.h"
#include "floats.h"
#include "fixints.h"
#include "rationals.h"
#include <math.h>

// Need to update this to handle bigints
#define CRACK(x) \
	double f_##x; \
	if (IsAFloat(x)) f_##x = DoubleFromFloat(x); \
	else if (IsAFixint(x)) f_##x = IntFromFixint(x); \
	else if (IsARational(x)) { \
		double fnumer = IntFromFixint( RationalNumerator(x) ); \
		double fdenom = IntFromFixint( RationalDenominator(x) ); \
		f_##x = fnumer / fdenom; \
	} else return ThrowCStr( zone, "expected a number" )

static value_t math_sin_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, sin( f_x ) );
}
struct closure math_sin = {(function_t)math_sin_func};

static value_t math_cos_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, cos( f_x ) );
}
struct closure math_cos = {(function_t)math_cos_func};

static value_t math_tan_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, tan( f_x ) );
}
struct closure math_tan = {(function_t)math_tan_func};

static value_t math_asin_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, asin( f_x ) );
}
struct closure math_asin = {(function_t)math_asin_func};

static value_t math_acos_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, acos( f_x ) );
}
struct closure math_acos = {(function_t)math_acos_func};

static value_t math_atan_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, atan( f_x ) );
}
struct closure math_atan = {(function_t)math_atan_func};

static value_t math_atan2_func( PREFUNC, value_t y, value_t x )
{
	ARGCHECK_2( y, x );
	CRACK(y);
	CRACK(x);
	return NumberFromDouble( zone, atan2( f_y, f_x ) );
}
struct closure math_atan2 = {(function_t)math_atan2_func};

static value_t math_sinh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, sinh( f_x ) );
}
struct closure math_sinh = {(function_t)math_sinh_func};

static value_t math_cosh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, cosh( f_x ) );
}
struct closure math_cosh = {(function_t)math_cosh_func};

static value_t math_tanh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, tanh( f_x ) );
}
struct closure math_tanh = {(function_t)math_tanh_func};

static value_t math_asinh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, asinh( f_x ) );
}
struct closure math_asinh = {(function_t)math_asinh_func};

static value_t math_acosh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, acosh( f_x ) );
}
struct closure math_acosh = {(function_t)math_acosh_func};

static value_t math_atanh_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, atanh( f_x ) );
}
struct closure math_atanh = {(function_t)math_atanh_func};

static value_t to_float_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromDouble( zone, f_x );
}
struct closure to_float = {(function_t)to_float_func};

static value_t floor_float_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromInt( zone, floor( f_x ) );
}
struct closure floor_float = {(function_t)floor_float_func};

static value_t ceiling_float_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromInt( zone, ceil( f_x ) );
}
struct closure ceiling_float = {(function_t)ceiling_float_func};

static value_t truncate_float_func( PREFUNC, value_t x )
{
	ARGCHECK_1( x );
	CRACK(x);
	return NumberFromInt( zone, trunc( f_x ) );
}
struct closure truncate_float = {(function_t)truncate_float_func};


