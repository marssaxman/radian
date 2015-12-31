// Copyright 2011-2014 Mars Saxman
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
#include "list-chunks.h"
#include "lists.h"
#include "exceptions.h"
#include "booleans.h"
#include "numbers.h"
#include "symbols.h"
#include "tuples.h"
#include "macros.h"

static value_t Single_function( PREFUNC, value_t selector );
#define SINGLE_SLOT_COUNT 1
#define SINGLE_VALUE_SLOT 0

static value_t Single_push( PREFUNC, value_t listObj,
	value_t value )
{
	ARGCHECK_2( listObj, value );
	// Pushing a new item onto the head means the existing value becomes the
	// tail. We will create a new multi-item list where the new item is the
	// sole item on the left chunk and the old item is the sole item on the
	// right chunk, and the central cold storage is the empty list.
	return AllocTwoItemList( zone, value, listObj->slots[SINGLE_VALUE_SLOT] );
}

static value_t Single_append( PREFUNC, value_t listObj,
	value_t value )
{
	ARGCHECK_2( listObj, value );
	// Appending a new item means the existing value becomes the head and the
	// new one becomes the tail. We will create a new multi-item list with
	// the head value in the left chunk and the tail value in the right chunk.
	return AllocTwoItemList( zone, listObj->slots[SINGLE_VALUE_SLOT], value );
}

static value_t Single_chpop( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Removing the only item in the list empties it, so all we need to do is
	// return the blank list again.
	return &list_empty;
}

static value_t Single_value( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// There's only one value in the list, so whether the caller is looking for
	// head or tail, the answer is the same.
	return listObj->slots[SINGLE_VALUE_SLOT];
}

static value_t Single_size( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Single-item list is always one item long, but that item may be a chunk
	// containing multiple leaf values.
	value_t val = listObj->slots[SINGLE_VALUE_SLOT];
	int leafs = 1;
	if (IsAChunk( val )) {
		leafs = chunk_leaf_count( val );
	}
	return NumberFromInt( zone, leafs );
}

static value_t Single_insert( PREFUNC, value_t listObj,
	value_t index, value_t value)
{
	ARGCHECK_3( listObj, index, value );
	// We can only insert at position 0 (head) or 1 (tail).
	switch (IntFromFixint( index )) {
		case 0: return Single_push( zone, self, 2, listObj, value );
		case 1: return Single_append( zone, self, 2, listObj, value );
		default: return ThrowCStr( zone, "index out of bounds" );
	}
}

static value_t Single_remove( PREFUNC, value_t listObj, value_t index )
{
	ARGCHECK_2( listObj, index );
	// There is only one valid index in this list: zero. Removing it empties
	// the list, which takes us back to the blank state.
	if (0 == IntFromFixint(index)) {
		return &list_empty;
	}
	return ThrowCStr( zone, "index out of bounds" );
}

static value_t Single_iter_fail( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	return ThrowCStr( zone, "the iterator is not valid" );
}

static value_t Single_iter_term( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (sym_is_valid == selector) return &False_returner;
	DEFINE_METHOD(next, Single_iter_fail)
	DEFINE_METHOD(current, Single_iter_fail)
	return ThrowCStr( zone, "the iterator does not have that method" );
}

static value_t Single_iter_current( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return iter->slots[0];
}

static value_t Single_iter_next( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	static struct closure out = {(function_t)Single_iter_term};
	return &out;
}

static value_t Single_iter_valid( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(next, Single_iter_next)
	DEFINE_METHOD(current, Single_iter_current)
	if (sym_is_valid == selector) return &True_returner;
	return ThrowCStr( zone, "the iterator does not have that method" );
}

static value_t Single_iterate( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	struct closure *out = ALLOC( Single_iter_valid, 1 );
	out->slots[0] = listObj->slots[SINGLE_VALUE_SLOT];
	return out;
}

static value_t Single_reverse( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// reverse of a single-element array is... the same array!
	return listObj;
}

static value_t Single_partition( PREFUNC, value_t listObj,
	value_t indexObj )
{
	ARGCHECK_2( listObj, indexObj );
	// A single-item list has only one unit of granularity. We will count up
	// the number of leaves it contains. If the split index is equal to this
	// number, our head list will be self and our tail will be empty; otherwise
	// our head list will be empty and our tail will be self. The partition
	// rule is that the tail must and the head must not contain the indexed
	// leaf value.
	int elements = IntFromFixint( METHOD_0( listObj, sym_size ) );
	int index = IntFromFixint( indexObj );
	if (index < 0 || index > elements) {
		return ThrowCStr( zone, "index out of bounds" );
	}
	else if (index < elements) {
		return AllocPair( zone, &list_empty, listObj );
	}
	else {
		return AllocPair( zone, listObj, &list_empty );
	}
}

static value_t Single_concatenate( PREFUNC, value_t listObj,
	value_t otherList )
{
	ARGCHECK_2( listObj, otherList );
	// concatenating some other list onto a single-element list is the same as
	// pushing the single-element list's value onto the other list.
	return METHOD_1( otherList, sym_push, listObj->slots[SINGLE_VALUE_SLOT] );
}

static value_t Single_assign( PREFUNC, value_t listObj,
	value_t index, value_t value )
{
	ARGCHECK_3( listObj, index, value );
	// There is only one valid index in this list: zero. Removing it empties
	// the list, which takes us back to the blank state.
	if (0 == IntFromFixint(index)) {
		struct closure *out = ALLOC( Single_function, SINGLE_SLOT_COUNT );
		out->slots[SINGLE_VALUE_SLOT] = value;
		return out;
	}
	return ThrowCStr( zone, "index out of bounds" );
}

static value_t Single_lookup( PREFUNC, value_t listObj, value_t indexObj )
{
	ARGCHECK_2( listObj, indexObj );

	// A single-item list is always one item long, but that item may be a chunk
	// containing multiple leaf values, in which case we actually perform a
	// lookup within the chunk.
	value_t val = listObj->slots[SINGLE_VALUE_SLOT];
	if (IsAChunk( val )) {
		int index = IntFromFixint( indexObj );
		if (index < chunk_leaf_count( val )) {
			val = chunk_get_leaf( zone, val, index );
		} else {
			val = ThrowCStr( zone, "index out of bounds" );
		}
	} else {
		if (0 != IntFromFixint( indexObj )) {
			val = ThrowCStr( zone, "index out of bounds" );
		}
	}
	return val;
}

static value_t Single_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	// The single-element list contains exactly one value. Since there is only
	// one value, head and tail are all the same. Removing this value bumps us
	// back down to the empty list; adding a new value suddenly creates an
	// ordering, which pushes us up to the general multi-element list function.
	DEFINE_METHOD(push, Single_push)
	DEFINE_METHOD(pop, Single_chpop)
	DEFINE_METHOD(head, Single_value)
	DEFINE_METHOD(append, Single_append)
	DEFINE_METHOD(chop, Single_chpop)
	DEFINE_METHOD(tail, Single_value)
	DEFINE_METHOD(size, Single_size)
	if (sym_is_empty == selector) return &False_returner;
	DEFINE_METHOD(iterate, Single_iterate)
	DEFINE_METHOD(reverse, Single_reverse)
	DEFINE_METHOD(partition, Single_partition)
	DEFINE_METHOD(concatenate, Single_concatenate)
	DEFINE_METHOD(lookup, Single_lookup)
	DEFINE_METHOD(insert, Single_insert)
	DEFINE_METHOD(remove, Single_remove)
	DEFINE_METHOD(assign, Single_assign)
	return ThrowCStr( zone, "list does not implement that method" );
}

value_t AllocSingleItemList( zone_t zone, value_t value )
{
	struct closure *out = ALLOC( Single_function, SINGLE_SLOT_COUNT );
	out->slots[SINGLE_VALUE_SLOT] = value;
	return out;
}
