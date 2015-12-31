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

#include "list-reverse.h"
#include "list-empty.h"
#include "symbols.h"
#include "exceptions.h"
#include "booleans.h"
#include "numbers.h"
#include "tuples.h"
#include "macros.h"

static value_t List_reverse_func( PREFUNC, value_t selector );
#define LIST_REVERSE_SLOT_COUNT 1
#define LIST_REVERSE_LIST_SLOT 0

static value_t List_iterator_func( PREFUNC, value_t selector );
#define LIST_ITERATOR_SLOT_COUNT 1
#define LIST_ITERATOR_LIST_SLOT 0

// For each wrapper that modifies the list, instead of creating a new instance
// of our own wrapper, we ask the underlying list to reverse itself. It may
// choose to create a different sort of wrapper, and the wrapper-creation
// operation is no more expensive whether the list does it or we do it.

static value_t FlipIndex( zone_t zone, value_t listObj, value_t index )
{
	value_t size = METHOD_0( listObj, sym_size );
	value_t ubound = METHOD_1( size, sym_subtract, num_one );
	return METHOD_1( ubound, sym_subtract, index );
}

static value_t List_lookup( PREFUNC, value_t wrapper, value_t index )
{
	ARGCHECK_2( wrapper, index );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	index = FlipIndex( zone, listObj, index );
	return METHOD_1( listObj, sym_lookup, index );
}

static value_t List_push( PREFUNC, value_t wrapper, value_t val )
{
	ARGCHECK_2( wrapper, val );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	listObj = METHOD_1( listObj, sym_append, val );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_pop( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	listObj = METHOD_0( listObj, sym_chop );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_head( PREFUNC, value_t  wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	return METHOD_0( listObj, sym_tail );
}

static value_t List_append( PREFUNC, value_t wrapper, value_t val )
{
	ARGCHECK_2( wrapper, val );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	listObj = METHOD_1( listObj, sym_push, val );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_chop( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	listObj = METHOD_0( listObj, sym_pop );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_tail( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	return METHOD_0( listObj, sym_head );
}

static value_t List_size( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	return METHOD_0( listObj, sym_size );
}

static value_t List_iterator_current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t target = iterator->slots[LIST_ITERATOR_LIST_SLOT];
	return METHOD_0( target, sym_tail );
}

static value_t List_iterator_next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t target = iterator->slots[LIST_ITERATOR_LIST_SLOT];
	target = METHOD_0( target, sym_chop );
	if (BoolFromBoolean( zone, METHOD_0( target, sym_is_empty ) )) {
		// if the list is empty, it will return an invalid iterator, which is
		// exactly what we want right now.
		return METHOD_0( target, sym_iterate );
	}
	else {
		// the list is not yet empty, so we will return another reverse wrapper
		// iterator pointing at its current tail.
		struct closure *out = 
			ALLOC( List_iterator_func, LIST_ITERATOR_SLOT_COUNT );
		out->slots[LIST_ITERATOR_LIST_SLOT] = target;
		return out;
	}
}

static value_t List_iterator_func( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (sym_is_valid == selector) return &True_returner;
	DEFINE_METHOD(current, List_iterator_current)
	DEFINE_METHOD(next, List_iterator_next)
	return ThrowCStr( zone, "iterator does not have that method" );
}

static value_t List_iterate( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	// This is the whole point of the reversing wrapper, really. We want to be
	// able to iterate over a list backwards at no greater cost than iterating
	// over it forwards. We'll generate an iterator here that works exactly the
	// same way as a normal list iterator, except it is based on tail/chop
	// instead of head/pop.
	struct closure *out = ALLOC( List_iterator_func, LIST_ITERATOR_SLOT_COUNT );
	out->slots[LIST_ITERATOR_LIST_SLOT] = listObj;
	return out;
}

static value_t List_reverse( PREFUNC, value_t wrapper )
{
	ARGCHECK_1( wrapper );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	return listObj;
}

static value_t List_partition( PREFUNC, value_t wrapper, value_t indexObj)
{
	ARGCHECK_2( wrapper, indexObj );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	indexObj = FlipIndex( zone, listObj, indexObj );
	value_t splits = METHOD_1( listObj, sym_partition, indexObj );
	// We have split the wrapped method - now we need to reverse the resulting
	// lists and return them in opposite order.
	value_t head_list = CALL_1( splits, num_zero );
	value_t tail_list = CALL_1( splits, num_one );
	head_list = METHOD_0( head_list, sym_reverse );
	tail_list = METHOD_0( tail_list, sym_reverse );
	return AllocPair( zone, tail_list, head_list );
}

static value_t List_concatenate( PREFUNC, value_t wrapper, value_t other )
{
	ARGCHECK_2( wrapper, other );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	if (other->function == (function_t)List_reverse_func) {
		// Both lists have been reversed, so we'll concatenate the inner lists
		// in reverse order and then flip the result.
		value_t otherObj = other->slots[LIST_REVERSE_LIST_SLOT];
		value_t both = METHOD_1( otherObj, sym_concatenate, listObj );
		return METHOD_0( both, sym_reverse );
	}
	else {
		// The only way to efficiently concatenate a mixed-implementation pair
		// of lists would be to push knowledge of the reversed implementation
		// into the main concatenate function, or to partly reimplement that
		// function inside this file, using a lot of knowledge about the main
		// list implementation. Let's not do that; encapsulation is good, even
		// if it has a performance penalty in this case. We will simply reify
		// the reversed list, then perform a normal concatenation.
		value_t inverse = &list_empty;
		value_t iter = METHOD_0( listObj, sym_iterate );
		while (true) {
			value_t is_valid = METHOD_0( iter, sym_is_valid );
			if (!BoolFromBoolean( zone, is_valid )) break;
			value_t val = METHOD_0( iter, sym_current );
			inverse = METHOD_1( inverse, sym_push, val );
			iter = METHOD_0( iter, sym_next );
		}
		return METHOD_1( inverse, sym_concatenate, other );
	}
}

static value_t List_insert( PREFUNC, value_t wrapper,
	value_t indexObj, value_t val )
{
	ARGCHECK_3( wrapper, indexObj, val );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	indexObj = FlipIndex( zone, listObj, indexObj );
	listObj = METHOD_2( listObj, sym_insert, indexObj, val );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_remove( PREFUNC, value_t wrapper, value_t indexObj )
{
	ARGCHECK_2( wrapper, indexObj );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	indexObj = FlipIndex( zone, listObj, indexObj );
	listObj = METHOD_1( listObj, sym_remove, indexObj );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_assign( PREFUNC, value_t wrapper,
	value_t indexObj, value_t val )
{
	ARGCHECK_3( wrapper, indexObj, val );
	value_t listObj = wrapper->slots[LIST_REVERSE_LIST_SLOT];
	indexObj = FlipIndex( zone, listObj, indexObj );
	listObj = METHOD_2( listObj, sym_assign, indexObj, val );
	return METHOD_0( listObj, sym_reverse );
}

static value_t List_reverse_func( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	// Reversed interface to a list. This MUST implement all the same methods
	// as the full list.
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
	DEFINE_METHOD(lookup, List_lookup)
	DEFINE_METHOD(insert, List_insert)
	DEFINE_METHOD(remove, List_remove)
	DEFINE_METHOD(assign, List_assign);
	return ThrowMemberNotFound( zone, selector );
}

value_t AllocReversedList( zone_t zone, value_t listObj )
{
	struct closure *out = ALLOC( List_reverse_func, LIST_REVERSE_SLOT_COUNT );
	out->slots[LIST_REVERSE_LIST_SLOT] = listObj;
	return out;
}
