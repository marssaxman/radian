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

// Implementation of a generic indexed list container as a 2-3 finger tree.
// This plays the role an array might have in a language with mutable data.

// This is actually a cluster of object functions which implement the same set
// of methods, as follows:
// push - insert value at the beginning of the list; it becomes head
// pop - remove element zero, shifting back all remaining elements
// head - return value of element zero
// append - insert value after all other elements; it becomes tail
// chop - remove the tailmost element; size drops by one
// tail - return value of the last element in the list
// size - how many items in this list?
// is_empty - are there any items in the list, or is it empty?
// iterate - traverse all the values in the list
// reverse - flip the list backwards so the tail becomes the head
// partition - divide a list in two, before and after/including some index
// concatenate - join two lists together
// lookup - return element at specified index
// assign - replace value at a specified index
// insert - put a new value into the list at a specific index
// remove - delete the item at the specified index
// When the selector is an integer key, this means lookup.

// If you add a new method to this list, you must add it to all of the list
// implementations: not only the multi-element list, but the single-element
// list and the zero-element list. None of these implementations should offer
// any methods that the others do not.

// The implementations are:
// zero-element list
// one-element list
// multi-element list
// chunk
// reversed multi-element list wrapper

#include "lists.h"
#include "list-chunks.h"
#include "list-empty.h"
#include "list-single.h"
#include "list-reverse.h"
#include "exceptions.h"
#include "symbols.h"
#include "numbers.h"
#include "booleans.h"
#include "tuples.h"
#include "macros.h"


static value_t List_function( PREFUNC, value_t selector );
#define LIST_SLOT_COUNT 4
#define LIST_HEAD_CHUNK_SLOT 0
#define LIST_COLD_STORAGE_SLOT 1
#define LIST_TAIL_CHUNK_SLOT 2
#define LIST_SIZE_SLOT 3

static value_t List_iterator_func( PREFUNC, value_t selector );
#define LIST_ITERATOR_SLOT_COUNT 1
#define LIST_ITERATOR_LIST_SLOT 0

static value_t alloc_list(
	zone_t zone,
	value_t head_chunk,
	value_t cold_storage,
	value_t tail_chunk )
{
	struct closure *out = ALLOC( List_function, LIST_SLOT_COUNT );
	out->slots[LIST_HEAD_CHUNK_SLOT] = head_chunk;
	out->slots[LIST_COLD_STORAGE_SLOT] = cold_storage;
	out->slots[LIST_TAIL_CHUNK_SLOT] = tail_chunk;

	int leaves = 0;
	leaves += chunk_leaf_count( head_chunk );
	leaves += IntFromFixint( METHOD_0( cold_storage, sym_size ) );
	leaves += chunk_leaf_count( tail_chunk );
	out->slots[LIST_SIZE_SLOT] = NumberFromInt( zone, leaves );

	return out;
}

static value_t List_push( PREFUNC, value_t listObj, value_t value )
{
	ARGCHECK_2( listObj, value );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	// If our head chunk has space remaining, push this value onto it. If the
	// chunk is full, we will push it onto the cold storage list and create a
	// new, one-element chunk for the new value, which becomes our head chunk.
	// This way the cold storage is a list of chunks, not a list of values.
	if (!chunk_can_grow( head_chunk )) {
		// Pushing a chunk onto cold storage is a potentially O(log n)
		// operation, as is popping a chunk. Therefore instead of pushing the
		// whole chunk, we will push only the last three elements. Our new
		// head chunk will include the new value and the previous head. This
		// way, an inexpensive pop always follows an expensive push, and vice
		// versa, which is how we get amortized O(1) performance.
		value_t pushable = chunk_pop( zone, head_chunk );
		cold_storage = METHOD_1( cold_storage, sym_push, pushable );
		head_chunk = chunk_alloc_1( zone, chunk_head( head_chunk ) );
	}
	// Push the new value onto our head chunk, which may or may not have been
	// freshly created for the purpose.
	head_chunk = chunk_push( zone, head_chunk, value );

	return alloc_list( zone, head_chunk, cold_storage, tail_chunk );
}

static value_t List_pop( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	// Remove the head element from this list. If our head chunk is not minimal,
	// we can just pop the value from the head chunk and continue on. Otherwise,
	// popping the value from the head chunk will empty it, and then we must
	// pull a new chunk from our cold storage. If the cold storage is empty, we
	// will try to cannibalize an element from the tail chunk. But if the tail
	// chunk is minimal, that means we have only one element left, which means
	// we should drop back down to single-element list mode.
	if (chunk_can_shrink( head_chunk )) {
		head_chunk = chunk_pop( zone, head_chunk );
	}
	else {
		// We need a new head chunk. If cold storage is not empty, get a chunk
		// from cold storage and call it our new head.
		value_t empty_val = METHOD_0( cold_storage, sym_is_empty );
		if (!BoolFromBoolean( zone, empty_val )) {
			// The cold storage is not empty. yay, get a chunk from it.
			head_chunk = METHOD_0( cold_storage, sym_head );
			cold_storage = METHOD_0( cold_storage, sym_pop );
		}
		else if (chunk_can_shrink( tail_chunk )) {
			// Cold storage is empty, but there are still items left on our
			// tail. We'll poach one item from the tail and put it on our head.
			head_chunk = chunk_alloc_1( zone, chunk_head( tail_chunk ) );
			tail_chunk = chunk_pop( zone, tail_chunk );
		}
		else {
			// Cold storage is empty and the tail chunk is already minimal.
			// This means we have only one value left, which means we should
			// drop back to single-item mode.
			return AllocSingleItemList( zone, chunk_head( tail_chunk ) );
		}
	}

	return alloc_list( zone, head_chunk, cold_storage, tail_chunk );
}

static value_t List_head( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Head is always element zero of our left chunk, which must never be
	// empty - if we were going to have an empty chunk we would have to demote
	// ourselves to a single-element list.
	value_t chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	return chunk_head( chunk );
}

static value_t List_append( PREFUNC, value_t listObj, value_t value )
{
	ARGCHECK_2( listObj, value );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	// If our tail chunk has space remaining, append this value to it. If the
	// chunk is full, we will append it to the cold storage list, then create
	// a new, one-element chunk for our tail.
	if (!chunk_can_grow( tail_chunk)) {
		// Following the same amortization logic as our push operation, we will
		// not append the entire full chunk, but only the first three elements.
		// The current tail value will remain on the tail chunk, along with the
		// value we are about to append. This way we always follow an expensive
		// operation with a cheap one, which gives us the amortized O(1)
		// performance which is the whole point of using a finger tree.
		value_t appendable = chunk_chop( zone, tail_chunk );
		cold_storage = METHOD_1( cold_storage, sym_append, appendable );
		tail_chunk = chunk_alloc_1( zone, chunk_tail( zone, tail_chunk ) );	
	}
	tail_chunk = chunk_append( zone, tail_chunk, value );

	return alloc_list( zone, head_chunk, cold_storage, tail_chunk );
}

static value_t List_chop( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	// Remove the tail element from this list. If our tail chunk is not minimal,
	// we can just chop off its tail and call it done. Otherwise, this will
	// empty our tail chunk, so we must get a new tail chunk. We will try to
	// get a new chunk from the end of cold storage. If cold storage is empty,
	// we will try to get a single value from the tail of our head chunk. And
	// if even that fails, we will fall back to single-element list mode.
	if (chunk_can_shrink( tail_chunk )) {
		tail_chunk = chunk_chop( zone, tail_chunk );
	}
	else {
		// We are going to throw away the only item left in our tail chunk, so
		// we need a new tail chunk to replace it. If cold storage is not
		// empty, we'll get its tail, which is a whole chunk.
		value_t empty_val = METHOD_0( cold_storage, sym_is_empty );
		if (!BoolFromBoolean( zone, empty_val )) {
			// The cold storage is not empty. yay, get its tail and then chop
			// it off so we don't try to use it twice.
			tail_chunk = METHOD_0( cold_storage, sym_tail );
			cold_storage = METHOD_0( cold_storage, sym_chop );
		}
		else if (chunk_can_shrink( head_chunk )) {
			// Cold storage is empty, but there are still items left on our
			// head chunk. We'll poach one item and paste it on the tail.
			tail_chunk = chunk_alloc_1( zone, chunk_tail( zone, head_chunk ) );
			head_chunk = chunk_chop( zone, head_chunk );
		}
		else {
			// Cold storage is empty, the head chunk is empty, and the tail
			// chunk is already minimal. This means we have only one value left,
			// which means we should drop back to single-item mode.
			return AllocSingleItemList( zone, chunk_tail( zone, head_chunk ) );
		}
	}

	return alloc_list( zone, head_chunk, cold_storage, tail_chunk );
}

static value_t List_tail( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Tail is always the last element of our tail chunk. We will never have an
	// empty tail chunk; we will go back to
	value_t chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];
	return chunk_tail( zone, chunk );
}

static value_t List_size( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	return listObj->slots[LIST_SIZE_SLOT];
}

static value_t List_iterator_current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t target = iterator->slots[LIST_ITERATOR_LIST_SLOT];
	return METHOD_0( target, sym_head );
}

static value_t List_iterator_next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t target = iterator->slots[LIST_ITERATOR_LIST_SLOT];
	target = METHOD_0( target, sym_pop );
	return METHOD_0( target, sym_iterate );
}

static value_t List_iterator_func( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (sym_is_valid == selector) return &True_returner;
	DEFINE_METHOD(current, List_iterator_current)
	DEFINE_METHOD(next, List_iterator_next)
	return ThrowCStr( zone, "iterator does not have that method" );
}

static value_t List_iterate( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Create an iterator for this list. Our iteration strategy is simple:
	// since head and pop are both O(1), each round of the iterator will just
	// return the current head. To get the next iterator value, we'll pop the
	// head from the list and call iterate() on the next list. Eventually this
	// will get down to a single-item list, which has its own iterator, and
	// it will figure out how to stop the iteration.
	struct closure *out = ALLOC( List_iterator_func, LIST_ITERATOR_SLOT_COUNT );
	out->slots[LIST_ITERATOR_LIST_SLOT] = listObj;
	return out;
}

static value_t List_reverse( PREFUNC, value_t listObj )
{
	ARGCHECK_1( listObj );
	// Return a reversing wrapper for this list. That is, a new object which
	// reinterprets all of the normal list methods so that the underlying list
	// appears to run in reverse order. We don't actually reallocate the list;
	// we can simply reorder the API, which is an O(1) operation.
	return AllocReversedList( zone, listObj );
}

static value_t listify_chunk( zone_t zone, value_t chunk )
{
	value_t out = &list_empty;
	while (chunk_can_shrink( chunk )) {
		out = METHOD_1( out, sym_append, chunk_head( chunk ) );
		chunk = chunk_pop( zone, chunk );
	}
	out = METHOD_1( out, sym_append, chunk_head( chunk ) );
	return out;
}

static value_t List_partition( PREFUNC, value_t listObj, value_t indexObj )
{
	ARGCHECK_2( listObj, indexObj );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	int index = IntFromFixint( indexObj );
	int size = IntFromFixint( METHOD_0( listObj, sym_size ) );
	if (index < 0 || index > size) {
		return ThrowCStr( zone, "index out of bounds" );
	}

	// Our job is to partition the list into a head list and a tail list, where
	// the head does not contain the leaf value identified by the index, and
	// the first item in the tail does contain that leaf value. Items may not
	// be the same as leaves, because we may actually be an inner list serving
	// as cold storage for some outer list; all we need to do is get as close
	// as we can to
	value_t head_out = &list_empty;
	value_t tail_out = &list_empty;

	// If the index comes before the cold storage, we will call the list "tail"
	// and pop items from it onto a new, empty "head" list until the next pop
	// would include the requested leaf item onto the head.
	if (index < chunk_leaf_count( head_chunk )) {
		tail_out = listObj;
		while (index > 0) {
			value_t item = METHOD_0( tail_out, sym_head );
			int old_tail_size = IntFromFixint( METHOD_0( tail_out, sym_size ) );
			value_t temp_tail = METHOD_0( tail_out, sym_pop );
			int new_tail_size = IntFromFixint( METHOD_0( temp_tail, sym_size ) );
			int leaf_count = new_tail_size - old_tail_size;
			if (leaf_count <= index) {
				head_out = METHOD_1( head_out, sym_append, item );
				tail_out = temp_tail;
			}
			index -= leaf_count;
		}
	}

	// If the index comes after the cold storage, we will call the current list
	// "head" and chop items from it, pushing them onto a new, empty "tail"
	// list, until the tail contains the index item.
	else if (size - index <= chunk_leaf_count( tail_chunk )) {
		head_out = listObj;
		int target = size - index;
		while (IntFromFixint( METHOD_0( tail_out, sym_size ) ) < target) {
			value_t item = METHOD_0( head_out, sym_tail );
			head_out = METHOD_0( head_out, sym_chop );
			tail_out = METHOD_1( tail_out, sym_push, item );
		}
	}

	// If the leaf index lives in neither the head nor the tail chunk, it must
	// live in cold storage. We will split cold storage in half and use the two
	// new halves as our output lists.
	else {
		index -= chunk_leaf_count( head_chunk );
		indexObj = NumberFromInt( zone, index );
		value_t splits = METHOD_1( cold_storage, sym_partition, indexObj );
		value_t cold_head = CALL_1( splits, num_zero );
		value_t cold_tail = CALL_1( splits, num_one );

		// Make the output head list. We will use our head chunk and the head
		// portion of the cold list, which of course contains chunks. If the
		// cold_head is non-empty, we will simply pull our tail chunk from it.
		// Otherwise, we will have to turn the head chunk into its own list.
		if (BoolFromBoolean( zone, METHOD_0( cold_head, sym_is_empty ) )) {
			head_out = listify_chunk( zone, head_chunk );
		}
		else {
			value_t new_tail_chunk = METHOD_0( cold_head, sym_tail );
			cold_head = METHOD_0( cold_head, sym_chop );
			head_out = alloc_list( zone, head_chunk, cold_head, new_tail_chunk );
		}

		// Make the output tail list. We will combine the tail half of the
		// newly-partitioned cold storage list with our tail chunk. If the
		// cold tail is not empty, we will pop its first chunk as our new list
		// head chunk; otherwise we will turn our tail chunk into its own list.
		if (BoolFromBoolean( zone, METHOD_0( cold_tail, sym_is_empty ) )) {
			tail_out = listify_chunk( zone, tail_chunk );
		}
		else {
			value_t new_head_chunk = METHOD_0( cold_tail, sym_head );
			cold_tail = METHOD_0( cold_tail, sym_pop );
			tail_out = alloc_list( zone, new_head_chunk, cold_tail, tail_chunk );
		}

		// We've done the rough cut of the partition, but there may be a few
		// stragglers on the tail list, since our cold storage works in terms
		// of chunks and not of leaves. Move items from the head of our tail
		// list to the tail of our head list, until moving one more item would
		// put our index leaf onto the head list. We don't know, at this level,
		// whether we are moving leaves or chunks, so we will just have to try
		// it and see what we get.
		while (true) {
			value_t item = METHOD_0( tail_out, sym_head );
			value_t maybe_head = METHOD_1( head_out, sym_append, item );
			if (IntFromFixint( METHOD_0( maybe_head, sym_size ) ) > index) {
				break;
			}
			head_out = maybe_head;
			tail_out = METHOD_0( tail_out, sym_pop );
		}
	}

	// Assemble the head & tail lists we have created into a tuple, since that
	// is the idiom for returning multiple values from a function.
	return AllocPair( zone, head_out, tail_out );
}

#include <stdio.h>
static value_t List_concatenate(
		PREFUNC, value_t listObj, value_t otherList )
{
	ARGCHECK_2( listObj, otherList );
	// If the list to be concatenated is also a multi-item list, we will do an
	// efficient concatenation by taking advantage of our knowledge of its
	// internal structure. Otherwise, we will hope it is a sequence and append
	// each of its items.
	if (otherList->function == (function_t)List_function) {
		value_t ourHeadChunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
		value_t ourColdStorage = listObj->slots[LIST_COLD_STORAGE_SLOT];
		value_t ourTailChunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];
		value_t otherHeadChunk = otherList->slots[LIST_HEAD_CHUNK_SLOT];
		value_t otherColdStorage = otherList->slots[LIST_COLD_STORAGE_SLOT];
		value_t otherTailChunk = otherList->slots[LIST_TAIL_CHUNK_SLOT];
		// We need to put these chunks into cold storage somehow. There are
		// enough possibilities that it would be tedious to do it all with if-
		// statements. Instead, we have this matrix, which will map each of the
		// sixteen possible combinations down to one of six actions. The
		// horizontal axis represents the size of the other list's head chunk;
		// the vertical axis represents the size of our list's tail chunk.
		int actions[4][4] = {
			{0, 1, 2, 2},
			{0, 4, 4, 2},
			{3, 4, 4, 5},
			{3, 3, 5, 5}};
		int x_size = chunk_item_count(otherHeadChunk);
		int y_size = chunk_item_count(ourTailChunk);
		switch(actions[y_size][x_size]) {
			case 0: {
				// Append other chunk's head to our chunk. Append our chunk to
				// our cold storage; let the other chunk drop.
				value_t item = chunk_head( otherHeadChunk );
				ourTailChunk = chunk_append( zone, ourTailChunk, item );
				ourColdStorage = 
					METHOD_1( ourColdStorage, sym_append, ourTailChunk );
			} break;
			case 1: {
				// Push our chunk's tail onto the other chunk. Push the other
				// chunk onto its cold storage, then let ours drop.
				value_t item = chunk_tail( zone, ourTailChunk );
				otherHeadChunk = chunk_push( zone, otherHeadChunk, item );
				otherColdStorage =
					METHOD_1( otherColdStorage, sym_push, otherHeadChunk );
			} break;
			case 2: {
				// Append other chunk's head to our chunk. Append our chunk to
				// our cold storage and push the other chunk onto its.
				value_t item = chunk_head( otherHeadChunk );
				otherHeadChunk = chunk_pop( zone, otherHeadChunk );
				otherColdStorage =
					METHOD_1( otherColdStorage, sym_push, otherHeadChunk );
				ourTailChunk = chunk_append( zone, ourTailChunk, item );
				ourColdStorage =
					METHOD_1( ourColdStorage, sym_append, ourTailChunk );
			} break;
			case 3: {
				// Push our chunk's tail onto the other chunk. Put each chunk
				// into its own cold storage.
				value_t item = chunk_tail( zone, ourTailChunk );
				ourTailChunk = chunk_chop( zone, ourTailChunk );
				ourColdStorage =
					METHOD_1( ourColdStorage, sym_append, ourTailChunk );
				otherHeadChunk = chunk_push( zone, otherHeadChunk, item );
				otherColdStorage = 
					METHOD_1( otherColdStorage, sym_push, otherHeadChunk );
			} break;
			case 4: {
				// Put each chunk into its own cold storage.
				ourColdStorage =
					METHOD_1( ourColdStorage, sym_append, ourTailChunk );
				otherColdStorage =
					METHOD_1( otherColdStorage, sym_push, otherHeadChunk );
			} break;
			case 5: {
				// Split one item off of each chunk to create a new middle
				// chunk. Put each original chunk onto its own list, then add
				// the middle chunk to our cold storage.
				value_t item = chunk_tail( zone, ourTailChunk );
				ourTailChunk = chunk_chop( zone, ourTailChunk );
				ourColdStorage =
					METHOD_1( ourColdStorage, sym_append, ourTailChunk );

				value_t midChunk = chunk_alloc_1( zone, item );
				item = chunk_head( otherHeadChunk );
				otherHeadChunk = chunk_pop( zone, otherHeadChunk );
				otherColdStorage =
					METHOD_1( otherColdStorage, sym_push, otherHeadChunk );

				midChunk = chunk_append( zone, midChunk, item );
				ourColdStorage =
					METHOD_1( ourColdStorage, sym_append, midChunk );
			} break;
		}
		// Having dealt with the active chunks, we now have a head chunk for
		// the whole list, a tail chunk for the whole list, and two cold
		// storage lists. We will recursively concatenate the cold storage
		// lists, then assemble our output list from these pieces.
		value_t combined_storage =
			METHOD_1( ourColdStorage, sym_concatenate, otherColdStorage );
		listObj = alloc_list(
				zone, ourHeadChunk, combined_storage, otherTailChunk );
	}
	else {
		value_t iter = METHOD_0( otherList, sym_iterate );
		if (IsAnException( iter )) return iter;
		while (BoolFromBoolean( zone, METHOD_0( iter, sym_is_valid ) )) {
			value_t val = METHOD_0( iter, sym_current );
			listObj = METHOD_1( listObj, sym_append, val );
			iter = METHOD_0( iter, sym_next );
		}
	}
	return listObj;
}

static value_t List_insert( PREFUNC, value_t listObj,
	value_t indexObj, value_t value )
{
	ARGCHECK_3( listObj, indexObj, value );
	value_t splits = METHOD_1( listObj, sym_partition, indexObj );
	value_t head_list = CALL_1( splits, num_zero );
	value_t tail_list = CALL_1( splits, num_one );
	tail_list = METHOD_1( tail_list, sym_push, value );
	return METHOD_1( head_list, sym_concatenate, tail_list );
}

static value_t List_remove( PREFUNC, value_t listObj,
	value_t indexObj )
{
	ARGCHECK_2( listObj, indexObj );
	value_t splits = METHOD_1( listObj, sym_partition, indexObj );
	value_t head_list = CALL_1( splits, num_zero );
	value_t tail_list = CALL_1( splits, num_one );
	tail_list = METHOD_0( tail_list, sym_pop );
	return METHOD_1( head_list, sym_concatenate, tail_list );
}

static value_t List_lookup( PREFUNC, value_t listObj,
	value_t indexObj )
{
	ARGCHECK_2( listObj, indexObj );
	value_t head_chunk = listObj->slots[LIST_HEAD_CHUNK_SLOT];
	value_t cold_storage = listObj->slots[LIST_COLD_STORAGE_SLOT];
	value_t tail_chunk = listObj->slots[LIST_TAIL_CHUNK_SLOT];

	int index = IntFromFixint( indexObj );
	if (index < 0) return ThrowCStr( zone, "index out of bounds" );

	int head_leaves = chunk_leaf_count( head_chunk );
	if (index < chunk_leaf_count( head_chunk )) {
		return chunk_get_leaf( zone, head_chunk, index );
	}
	index -= head_leaves;

	int cold_leaves = IntFromFixint( METHOD_0( cold_storage, sym_size ) );
	if (index < cold_leaves) {
		return METHOD_1(
				cold_storage, sym_lookup, NumberFromInt( zone, index ) );
	}
	index -= cold_leaves;

	int tail_leaves = chunk_leaf_count( tail_chunk );
	if (index < chunk_leaf_count( tail_chunk )) {
		return chunk_get_leaf( zone, tail_chunk, index );
	}
	index -= tail_leaves;

	return ThrowCStr( zone, "index out of bounds" );
}

static value_t List_assign( PREFUNC, value_t listObj,
	value_t indexObj, value_t value )
{
	ARGCHECK_3( listObj, indexObj, value );
	value_t splits = METHOD_1( listObj, sym_partition, indexObj );
	value_t head_list = CALL_1( splits, num_zero );
	value_t tail_list = CALL_1( splits, num_one );
	tail_list = METHOD_0( tail_list, sym_pop );
	tail_list = METHOD_1( tail_list, sym_push, value );
	return METHOD_1( head_list, sym_concatenate, tail_list );
}

static value_t List_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(push, List_push)
	DEFINE_METHOD(pop, List_pop)
	DEFINE_METHOD(head, List_head)
	DEFINE_METHOD(append, List_append)
	DEFINE_METHOD(chop, List_chop)
	DEFINE_METHOD(tail, List_tail)
	DEFINE_METHOD(size, List_size)
	if (sym_is_empty == selector) return &False_returner;
	DEFINE_METHOD(iterate, List_iterate);
	DEFINE_METHOD(reverse, List_reverse);
	DEFINE_METHOD(partition, List_partition);
	DEFINE_METHOD(concatenate, List_concatenate);
	DEFINE_METHOD(lookup, List_lookup);
	DEFINE_METHOD(insert, List_insert)
	DEFINE_METHOD(remove, List_remove)
	DEFINE_METHOD(assign, List_assign);
	return ThrowMemberNotFound( zone, selector );
}

value_t AllocTwoItemList( zone_t zone, value_t head, value_t tail )
{
	return alloc_list(
		zone,
		chunk_alloc_1( zone, head ),
		&list_empty,
		chunk_alloc_1( zone, tail ) );
}

static value_t List_constructor( PREFUNC, value_t exp )
{
	ARGCHECK_1( exp );
	// This intrinsic function represents the brackets syntax for list
	// construction. Our exp is whatever was enclosed in the brackets.
	// This ought to be a tuple, or something which looks like one; each
	// element of the tuple will be a new entry in the array.

	value_t out = &list_empty;
	if (exp) {
		value_t item_count = METHOD_0( exp, sym_size );
		if (IsAnException( item_count )) return item_count;
		unsigned int items = IntFromFixint( item_count );
		for (unsigned int i = 0; i < items; i++) {
			value_t item = CALL_1( exp, NumberFromInt( zone, i ) );
			if (IsAnException( item )) return item;
			out = METHOD_1( out, sym_append, item );
		}
	}
	return out;
}

struct closure list = {(function_t)List_constructor};

