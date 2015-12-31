// Copyright 2011-2012 Mars Saxman
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

#include "list-empty.h"
#include "list-single.h"
#include "exceptions.h"
#include "booleans.h"
#include "numbers.h"
#include "symbols.h"
#include "tuples.h"
#include "macros.h"

static value_t Empty_function( PREFUNC, value_t selector );
#define EMPTY_SLOT_COUNT 0

struct closure list_empty = {(function_t)Empty_function};

static value_t Empty_fail( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	return ThrowCStr( zone, "the list is empty" );
}

static value_t Empty_add( PREFUNC, value_t listObj, value_t value )
{
	ARGCHECK_2( listObj, value );
	return AllocSingleItemList( zone, value );
}

static value_t Empty_size( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	return num_zero;
}

static value_t Empty_iterator( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (sym_is_valid == selector) return &False_returner;
	DEFINE_METHOD(next, Empty_fail)
	DEFINE_METHOD(current, Empty_fail)
	return ThrowCStr( zone, "the iterator does not have that method" );
}

static value_t Empty_iterate( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// we can iterate over an empty list, but all we get back is an invalid
	// (aka terminal) iterator.
	static struct closure out = {(function_t)Empty_iterator};
	return &out;
}

static value_t Empty_reverse( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// reverse of an empty list is also an empty list
	return listObj;
}

static value_t Empty_partition( PREFUNC, value_t listObj, value_t index )
{
	ARGCHECK_2( listObj, index );
	// The empty list contains no items or leaves, so the only index which
	// makes any sense is zero. All others are out of bounds. Return of a
	// partition is always a 2-tuple with the head and tail; we will return
	// both as empty.
	if (IntFromFixint( index ) == 0) {
		return AllocPair( zone, &list_empty, &list_empty );
	}
	else {
		return ThrowCStr( zone, "index out of bounds" );
	}
}

static value_t Empty_concatenate( PREFUNC, value_t listObj,
	value_t otherList )
{
	ARGCHECK_2( listObj, otherList );
	// concatenating an empty list with anything equals the other thing
	return otherList;
}

static value_t Empty_insert( PREFUNC, value_t listObj,
	value_t index, value_t value )
{
	ARGCHECK_3( listObj, index, value );
	if (IntFromFixint(index) != 0) {
		return ThrowCStr( zone, "index out of bounds" );
	}
	return AllocSingleItemList( zone, value );
}

static value_t Empty_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	// The empty list has no items, so most of its methods fail. The only ones
	// which actually work are the ones that ask for information about size and
	// the ones that add an entry to the list. Adding an entry to the empty
	// list pushes us up to the next category: the single-element list.
	DEFINE_METHOD(push, Empty_add)
	DEFINE_METHOD(pop, Empty_fail)
	DEFINE_METHOD(head, Empty_fail)
	DEFINE_METHOD(append, Empty_add)
	DEFINE_METHOD(chop, Empty_fail)
	DEFINE_METHOD(tail, Empty_fail)
	DEFINE_METHOD(size, Empty_size)
	if (sym_is_empty == selector) return &True_returner;
	DEFINE_METHOD(iterate, Empty_iterate)
	DEFINE_METHOD(reverse, Empty_reverse)
	DEFINE_METHOD(partition, Empty_partition)
	DEFINE_METHOD(concatenate, Empty_concatenate)
	DEFINE_METHOD(lookup, Empty_fail)
	DEFINE_METHOD(insert, Empty_insert)
	DEFINE_METHOD(remove, Empty_fail)
	DEFINE_METHOD(assign, Empty_fail)
	return ThrowMemberNotFound( zone, selector );
}
