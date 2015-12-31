// Copyright 2012-2013 Mars Saxman
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
#include "relations.h"
#include "exceptions.h"
#include "macros.h"

static value_t LessThan_function( PREFUNC, 
	value_t less, value_t equal, value_t greater ) { return less; }
static struct closure s_less_than = {(function_t)LessThan_function};

value_t LessThan(void)
{
	return &s_less_than;
}

static value_t Equal_function( PREFUNC, 
	value_t less, value_t equal, value_t greater ) { return equal; }
static struct closure s_equal = {(function_t)Equal_function};

value_t EqualTo(void)
{
	return &s_equal;
}

static value_t GreaterThan_function( PREFUNC, 
	value_t less, value_t equal, value_t greater ) { return greater; }
static struct closure s_greater_than = {(function_t)GreaterThan_function};

value_t GreaterThan(void)
{
	return &s_greater_than;
}

int IntFromRelation( zone_t zone, value_t relation )
{
	assert( !IsAnException( relation ) );
	// A relation is a function which returns one of its three arguments. It's
	// possible that the relation is one of the stock relations we've defined
	// here, in which case we can perform a trivial pointer check. Otherwise,
	// we can convert a relation to an int by passing specific arguments in and
	// seeing which one comes back out.
	if (relation == &s_less_than) return -1;
	if (relation == &s_equal) return 0;
	if (relation == &s_greater_than) return 1;
	relation = CALL_3( relation, &s_less_than, &s_equal, &s_greater_than );
	if (relation == &s_less_than) return -1;
	if (relation == &s_equal) return 0;
	if (relation == &s_greater_than) return 1;
	assert( false );
}

value_t RelationFromInt( int rel )
{
	if (rel < 0) return LessThan();
	if (rel > 0) return GreaterThan();
	return EqualTo();
}

value_t RelationFromDouble( double rel )
{
	if (rel < 0) return LessThan();
	if (rel > 0) return GreaterThan();
	return EqualTo();
}

value_t InvertRelation( zone_t zone, value_t relation )
{
	return CALL_3( relation, GreaterThan(), EqualTo(), LessThan() );
}
