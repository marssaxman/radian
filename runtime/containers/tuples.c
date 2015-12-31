// Copyright 2009-2012 Mars Saxman
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


#include "tuples.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include "numbers.h"
#include "symbols.h"
#include "exceptions.h"
#include "../atoms/stringliterals.h"
#include "booleans.h"
#include "macros.h"

#define TUPLE_SLOT_COUNT 1
#define TUPLE_SIZE_SLOT 0

#define TUPLE_ITERATOR_SLOT_COUNT 2
#define TUPLE_ITERATOR_TARGET_SLOT 0
#define TUPLE_ITERATOR_INDEX_SLOT 1

static value_t Tuple_iterator_function( PREFUNC, value_t iterator );

// Implementations of the tuple object methods:

static value_t Tuple_size_returner( PREFUNC, value_t tuple )
{
	ARGCHECK_1( tuple );
	return tuple->slots[TUPLE_SIZE_SLOT];
}

static value_t Tuple_is_empty( PREFUNC, value_t tuple )
{
	ARGCHECK_1( tuple );
	unsigned int size = IntFromFixint( tuple->slots[TUPLE_SIZE_SLOT] );
	return BooleanFromBool( size > 0 );
}

static value_t Tuple_lookup( PREFUNC, value_t tuple, value_t selector )
{
	ARGCHECK_2( tuple, selector );
	size_t elements = IntFromFixint( tuple->slots[TUPLE_SIZE_SLOT] );
	if (!IsAFixint( selector )) {
		return ThrowCStr( zone, "element index is not an integer" );
	}
	size_t index = IntFromFixint( selector );
	if (index >= elements) {
		return ThrowCStr( zone, "element index is out of bounds" );
	}
	return tuple->slots[TUPLE_SLOT_COUNT + index];
}

static value_t Tuple_iterate( PREFUNC, value_t tuple )
{
	ARGCHECK_1( tuple );
	struct closure *out =
			ALLOC( Tuple_iterator_function, TUPLE_ITERATOR_SLOT_COUNT );
	out->slots[TUPLE_ITERATOR_TARGET_SLOT] = tuple;
	out->slots[TUPLE_ITERATOR_INDEX_SLOT] = num_zero;
	return out;
}

static value_t Tuple_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (IsAFixint( selector )) {
		// tuple, unlike other containers, can be invoked with an index value,
		// instead of using the lookup method. This was the original scheme for
		// all subscript lookups, but after implementing the other containers
		// it came to look like a bad plan. The compiler uses a lot of tuples
		// internally to implement flow control structures, so we will leave
		// the old lookup behavior intact for speedy internal use.
		static struct closure lookerupper = {(function_t)Tuple_lookup};
		return Tuple_lookup( zone, &lookerupper, 2, self, selector );
	}
	DEFINE_METHOD(is_empty, Tuple_is_empty)
	DEFINE_METHOD(iterate, Tuple_iterate)
	DEFINE_METHOD(size, Tuple_size_returner)
	DEFINE_METHOD(lookup, Tuple_lookup)
	return ThrowCStr( zone, "tuple does not have that member" );
}

static value_t Tuple_iterator_current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t tuple = iterator->slots[TUPLE_ITERATOR_TARGET_SLOT];
	value_t index = iterator->slots[TUPLE_ITERATOR_INDEX_SLOT];
	return METHOD_1( tuple, sym_lookup, index );
}

static value_t Tuple_iterator_valid( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t tuple = iterator->slots[TUPLE_ITERATOR_TARGET_SLOT];
	size_t elements = IntFromFixint( tuple->slots[TUPLE_SIZE_SLOT] );
	size_t index = IntFromFixint( iterator->slots[TUPLE_ITERATOR_INDEX_SLOT] );
	return BooleanFromBool( index < elements );
}

static value_t Tuple_iterator_next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t index = iterator->slots[TUPLE_ITERATOR_INDEX_SLOT];
	struct closure *out =
			ALLOC(Tuple_iterator_function, TUPLE_ITERATOR_SLOT_COUNT);
	out->slots[TUPLE_ITERATOR_TARGET_SLOT] =
			iterator->slots[TUPLE_ITERATOR_TARGET_SLOT];
	out->slots[TUPLE_ITERATOR_INDEX_SLOT] =
			METHOD_1( index, sym_add, num_one );
	return out;
}

static value_t Tuple_iterator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(current, Tuple_iterator_current);
	DEFINE_METHOD(is_valid, Tuple_iterator_valid);
	DEFINE_METHOD(next, Tuple_iterator_next);
	return ThrowMemberNotFound( zone, selector );
}


static value_t Tuple_constructor( PREFUNC, ... )
{
	va_list vl;
	va_start(vl, argc);
	unsigned int count = argc;
	struct closure *out = ALLOC( Tuple_function, count + TUPLE_SLOT_COUNT );
	out->slots[TUPLE_SIZE_SLOT] = NumberFromInt( zone, count );
	for (unsigned int i = 0; i < count; i++) {
		out->slots[i + TUPLE_SLOT_COUNT] = va_arg(vl, value_t);
	}
	return out;
}

struct closure make_tuple = {(function_t)Tuple_constructor};

value_t AllocPair( zone_t zone, value_t zero, value_t one )
{
	struct closure *out = ALLOC( Tuple_function, 2 + TUPLE_SLOT_COUNT );
	out->slots[TUPLE_SIZE_SLOT] = NumberFromInt( zone, 2 );
	out->slots[0 + TUPLE_SLOT_COUNT] = zero;
	out->slots[1 + TUPLE_SLOT_COUNT] = one;
	return out;
}

value_t AllocQuadruple(
		zone_t zone, value_t i0, value_t i1, value_t i2, value_t i3 )
{
	struct closure *out = ALLOC( Tuple_function, 4 + TUPLE_SLOT_COUNT );
	out->slots[TUPLE_SIZE_SLOT] = NumberFromInt( zone, 4 );
	out->slots[0 + TUPLE_SLOT_COUNT] = i0;
	out->slots[1 + TUPLE_SLOT_COUNT] = i1;
	out->slots[2 + TUPLE_SLOT_COUNT] = i2;
	out->slots[3 + TUPLE_SLOT_COUNT] = i3;
	return out;
}

bool IsATuple( value_t exp )
{
	return exp && exp->function == (function_t)Tuple_function;
}


