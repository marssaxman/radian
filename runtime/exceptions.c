// Copyright 2010-2013 Mars Saxman
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


#include <assert.h>
#include "exceptions.h"
#include "booleans.h"
#include "atoms/stringliterals.h"
#include "tuples.h"
#include "numbers.h"
#include "macros.h"

static value_t Exception_function( PREFUNC )
{
	// An exception has the entertaining property of always returning itself,
	// no matter what parameters you call it with. This is key to its ability
	// to contaminate any expression it comes into contact with, propagating
	// the knowledge of some error state (and the according invalidity of any
	// values computed on erroneous premises) up to some caller that can handle
	// the situation.
	return self;
}

value_t Throw( zone_t zone, value_t exp )
{
	struct closure *out = ALLOC( Exception_function, 1 );
	out->slots[0] = exp;
	return out;
}

value_t ThrowCStr( zone_t zone, const char *msg )
{
	return Throw( zone, StringFromCStr( zone, msg ) );
}

value_t ThrowCStrNotFound( zone_t zone, const char *msg, value_t selector )
{
	value_t message = StringFromCStr( zone, msg );
	return Throw( zone, AllocPair( zone, message, selector ) );
}

value_t ThrowMemberNotFound( zone_t zone, value_t sym )
{
    return ThrowCStrNotFound( zone, "member not found", sym );
}

// ThrowArgCountFail
//
// The prolog for every function checks the supplied arg count against the
// expected arg count and calls this function if they don't match. We must
// alloc a suitable exception. The name of this function is hardcoded into
// llvmatom.cpp, so go change the constant there if you rename this.
//
value_t ThrowArgCountFail(
		zone_t zone, const char *function, int32_t expected, int32_t actual )
{
	assert( expected != actual );
	value_t message = StringFromCStr(
			zone,
			expected < actual ? "too many arguments" : "not enough arguments" );
	value_t function_name_obj = StringFromCStr( zone, function );
	value_t expected_obj = NumberFromInt( zone, expected );
	value_t actual_obj = NumberFromInt( zone, actual );
	value_t content = AllocQuadruple(
			zone, message, function_name_obj, expected_obj, actual_obj );
	return Throw( zone, content );
}

value_t ExceptionContents( value_t exception )
{
    assert( IsAnException( exception ) );
    return exception->slots[0];
}

static value_t throw_exception_func( PREFUNC, value_t exp )
{
	ARGCHECK_1( exp );
	return Throw( zone, exp );
}
struct closure throw_exception = {(function_t)throw_exception_func};

bool IsAnException( value_t exp )
{
	// Is it one of ours?
	return exp && exp->function == (function_t)Exception_function;
}

static value_t catch_exception_func( PREFUNC, value_t exp, value_t handler )
{
	ARGCHECK_2( NULL, handler );
	if (IsAnException( exp )) {
		value_t val = exp->slots[0];
		assert( handler );
		exp = CALL_1( handler, val );
	}
	return exp;
}
struct closure catch_exception = {(function_t)catch_exception_func};

static value_t is_not_exceptional_func( PREFUNC, value_t exp )
{
	ARGCHECK_1( NULL );
	return BooleanFromBool( !IsAnException( exp ) );
}
struct closure is_not_exceptional = {(function_t)is_not_exceptional_func};

