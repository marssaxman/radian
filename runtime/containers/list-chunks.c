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

// Support module for the list type. List chunks are small, fixed-size 
// deques used as components of the finger tree. A chunk contains exactly 
// 1, 2, 3, or 4 items. There are no empty chunks and there are no chunks with
// more than 4 items. We distinguish between "items", which are the values held
// directly by a chunk, and "leaves", which are the data units the finger tree
// manages overall. A chunk may hold leaves, but it may also hold other chunks,
// since the finger tree is a recursive data structure. In either case, items
// or leaves, we store them in ascending index order, but sometimes we need to
// look things up by item index (which is always 0, 1, 2, or 3) and sometimes
// we need to look things up by leaf index (which may be anywhere from 0 to
// some bound defined by available memory).

#include "list-chunks.h"
#include "exceptions.h"
#include "tuples.h"
#include "symbols.h"
#include "numbers.h"
#include "macros.h"

#define CHUNK_SLOT_COUNT 1
#define CHUNK_LEAF_COUNT_SLOT 0

static value_t One_function( PREFUNC, value_t selector )
	{ return ThrowCStr( zone, "assert" ); }

static value_t Two_function( PREFUNC, value_t selector )
	{ return ThrowCStr( zone, "assert" ); }

static value_t Three_function( PREFUNC, value_t selector )
	{ return ThrowCStr( zone, "assert" ); }

static value_t Four_function( PREFUNC, value_t selector )
	{ return ThrowCStr( zone, "assert" ); }

bool IsAChunk( value_t something )
{
	return 
		something->function == (function_t)One_function ||
		something->function == (function_t)Two_function ||
		something->function == (function_t)Three_function ||
		something->function == (function_t)Four_function;
}

static int LeavesInValue( value_t val )
{
	return IsAChunk( val ) ? chunk_leaf_count( val ): 1;
}

value_t chunk_alloc_1( zone_t zone, value_t value )
{
	struct closure *out = ALLOC( One_function, CHUNK_SLOT_COUNT + 1 );
	int leaves = 0;
	leaves += LeavesInValue( value );
	out->slots[CHUNK_LEAF_COUNT_SLOT] = NumberFromInt( zone, leaves );
	out->slots[CHUNK_SLOT_COUNT + 0] = value;
	return out;
}

static value_t chunk_alloc_2( zone_t zone, value_t val1, value_t val2 )
{
	struct closure *out = ALLOC( Two_function, CHUNK_SLOT_COUNT + 2 );
	int leaves = 0;
	leaves += LeavesInValue( val1 );
	leaves += LeavesInValue( val2 );
	out->slots[CHUNK_LEAF_COUNT_SLOT] = NumberFromInt( zone, leaves );
	out->slots[CHUNK_SLOT_COUNT + 0] = val1;
	out->slots[CHUNK_SLOT_COUNT + 1] = val2;
	return out;
}

static value_t chunk_alloc_3( 
	zone_t zone, 
	value_t val1, 
	value_t val2, 
	value_t val3 )
{
	struct closure *out = ALLOC( Three_function, CHUNK_SLOT_COUNT + 3 );
	int leaves = 0;
	leaves += LeavesInValue( val1 );
	leaves += LeavesInValue( val2 );
	leaves += LeavesInValue( val3 );
	out->slots[CHUNK_LEAF_COUNT_SLOT] = NumberFromInt( zone, leaves );
	out->slots[CHUNK_SLOT_COUNT + 0] = val1;
	out->slots[CHUNK_SLOT_COUNT + 1] = val2;
	out->slots[CHUNK_SLOT_COUNT + 2] = val3;
	return out;
}

static value_t chunk_alloc_4( 
	zone_t zone,
	value_t val1, 
	value_t val2, 
	value_t val3, 
	value_t val4 )
{
	struct closure *out = ALLOC( Four_function, CHUNK_SLOT_COUNT + 4 );
	int leaves = 0;
	leaves += LeavesInValue( val1 );
	leaves += LeavesInValue( val2 );
	leaves += LeavesInValue( val3 );
	leaves += LeavesInValue( val4 );
	out->slots[CHUNK_LEAF_COUNT_SLOT] = NumberFromInt( zone, leaves );
	out->slots[CHUNK_SLOT_COUNT + 0] = val1;
	out->slots[CHUNK_SLOT_COUNT + 1] = val2;
	out->slots[CHUNK_SLOT_COUNT + 2] = val3;
	out->slots[CHUNK_SLOT_COUNT + 3] = val4;
	return out;
}

int chunk_item_count( value_t chunk )
{
	// How many items does this chunk contain? This is not the same as the 
	// number of leaf values. A chunk may contain other chunks, which contain
	// data elements; all we care about here is the chunk's own tuple order.
	if (chunk->function == (function_t)One_function) return 1;
	if (chunk->function == (function_t)Two_function) return 2;
	if (chunk->function == (function_t)Three_function) return 3;
	if (chunk->function == (function_t)Four_function) return 4;
	return 0;
}

int chunk_leaf_count( value_t chunk )
{
	// How many leaves does the chunk actually contain? We computed this when
	// we created the chunk, so we can simply return the value now.
	return IntFromFixint( chunk->slots[CHUNK_LEAF_COUNT_SLOT] );
}

static value_t chunk_item_0( value_t chunk ) 
	{ return chunk->slots[CHUNK_SLOT_COUNT + 0]; }

static value_t chunk_item_1( value_t chunk ) 
	{ return chunk->slots[CHUNK_SLOT_COUNT + 1]; }

static value_t chunk_item_2( value_t chunk ) 
	{ return chunk->slots[CHUNK_SLOT_COUNT + 2]; }

static value_t chunk_item_3( value_t chunk ) 
	{ return chunk->slots[CHUNK_SLOT_COUNT + 3]; }

bool chunk_can_grow( value_t chunk )
{
	// A chunk can grow if it contains fewer than 4 items.
	return chunk_item_count( chunk ) < 4;
}

bool chunk_can_shrink( value_t chunk )
{
	// A chunk can shrink down to 1 item but no further.
	return chunk_item_count( chunk ) > 1;
}

value_t chunk_push( zone_t zone, value_t chunk, value_t value )
{
	// Push a new value onto the head of a chunk which already contains some
	// items. We will never encounter an empty chunk and will never try to
	// push an item onto a full chunk.
	switch (chunk_item_count( chunk )) {
		case 1: return chunk_alloc_2( 
			zone,
			value,
			chunk_item_0( chunk ));
		case 2: return chunk_alloc_3( 
			zone,
			value,
			chunk_item_0( chunk ), 
			chunk_item_1( chunk ) );
		case 3: return chunk_alloc_4(
			zone,
			value,
			chunk_item_0( chunk ),
			chunk_item_1( chunk ),
			chunk_item_2( chunk ) );
		default: return ThrowCStr( zone, "assert" );
	}
}

value_t chunk_pop( zone_t zone, value_t chunk )
{
	// Remove the headmost item from a chunk. We will never have to deal with
	// a minimal chunk, or a chunk of more than 4 items.
	switch (chunk_item_count( chunk )) {
		case 2: return chunk_alloc_1(
			zone,
			chunk_item_1( chunk ) );
		case 3: return chunk_alloc_2(
			zone,
			chunk_item_1( chunk ),
			chunk_item_2( chunk ) );
		case 4: return chunk_alloc_3(
			zone,
			chunk_item_1( chunk ),
			chunk_item_2( chunk ),
			chunk_item_3( chunk ) );
		default: return ThrowCStr( zone, "assert" );
	}
}

value_t chunk_head( value_t chunk )
{
	return chunk_item_0( chunk );
}

value_t chunk_append( zone_t zone, value_t chunk, value_t value )
{
	// Append a new value to the tail of a chunk which already contains some
	// items. We will never encounter an empty chunk and will never try to
	// append an item to a chunk which is already full.
	switch (chunk_item_count( chunk )) {
		case 1: return chunk_alloc_2(
			zone,
			chunk_item_0( chunk ),
			value );
		case 2: return chunk_alloc_3(
			zone,
			chunk_item_0( chunk ),
			chunk_item_1( chunk ),
			value );
		case 3: return chunk_alloc_4(
			zone,
			chunk_item_0( chunk ),
			chunk_item_1( chunk ),
			chunk_item_2( chunk ),
			value );
		default: return ThrowCStr( zone, "assert" );
	}
}

value_t chunk_chop( zone_t zone, value_t chunk )
{
	// Remove the tailmost element from this chunk. We will never have to deal 
	// with a minimal chunk, or a chunk of more than 4 items.
	switch (chunk_item_count( chunk )) {
		case 2: return chunk_alloc_1(
			zone,
			chunk_item_0( chunk ) );
		case 3: return chunk_alloc_2(
			zone,
			chunk_item_0( chunk ),
			chunk_item_1( chunk ) );
		case 4: return chunk_alloc_3(
			zone,
			chunk_item_0( chunk ),
			chunk_item_1( chunk ),
			chunk_item_2( chunk ) );
		default: return ThrowCStr( zone, "assert" );
	}
}

value_t chunk_tail( zone_t zone, value_t chunk )
{
	switch (chunk_item_count( chunk )) {
		case 1: return chunk_item_0( chunk );
		case 2: return chunk_item_1( chunk );
		case 3: return chunk_item_2( chunk );
		case 4: return chunk_item_3( chunk );
		default: return ThrowCStr( zone, "assert" );
	}
}

value_t chunk_get_leaf( zone_t zone, value_t chunk, int index )
{
	value_t val = chunk_item_0( chunk );
	if (IsAChunk( val )) {
		int leaves = chunk_leaf_count( val );
		if (index < leaves) {
			return chunk_get_leaf( zone, val, index );
		}
		index -= leaves;
	}
	else {
		if (index < 1) return val;
		index -= 1;
	}

	if (chunk_item_count( chunk ) < 2) {
		return ThrowCStr( zone, "index out of bounds" );
	}
	val = chunk_item_1( chunk );
	if (IsAChunk( val )) {
		int leaves = chunk_leaf_count( val );
		if (index < leaves) {
			return chunk_get_leaf( zone, val, index );
		}
		index -= chunk_leaf_count( val );
	}
	else {
		if (index < 1) return val;
		index -= 1;
	}
	
	if (chunk_item_count( chunk ) < 3) {
		return ThrowCStr( zone, "index out of bounds" );
	}
	val = chunk_item_2( chunk );
	if (IsAChunk( val )) {
		int leaves = chunk_leaf_count( val );
		if (index < leaves) {
			return chunk_get_leaf( zone, val, index );
		}
		index -= chunk_leaf_count( val );
	}
	else {
		if (index < 1) return val;
		index -= 1;
	}
	
	if (chunk_item_count( chunk ) < 4) {
		return ThrowCStr( zone, "index out of bounds" );
	}
	val = chunk_item_3( chunk );
	if (IsAChunk( val )) {
		int leaves = chunk_leaf_count( val );
		if (index < leaves) {
			return chunk_get_leaf( zone, val, index );
		}
		index -= chunk_leaf_count( val );
	}
	else {
		if (index < 1) return val;
		index -= 1;
	}
	
	return ThrowCStr( zone, "index out of bounds" );
}

