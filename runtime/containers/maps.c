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


// This is an immutable implementation of the AA-tree, or Andersson tree, a
// type of self-balancing binary tree based on the 2-3 B-tree. This is a
// simpler algorithm than the more common red-black tree, which is based on the
// 2-3-4 b-tree. Its insert performance is comparable, but remove is more
// complex; on the other hand it is comprehensible by mere mortals.

// Consider adding a 'comparator' property which lets you specify some function
// to perform comparisons. By default this would be a function which invokes
// the comparable interface, but it might be nice if you could sort things in
// some other order.

// Map implements the 'collection' interface, which incorporates 'container'
// and 'sequence'. It should expose these methods:
//	iterate, size, is_empty, lookup, insert, remove, contains

// Objects and modules use maps as their member indexes. The member dispatch
// code expects map to have methods named "insert", "assign", and "lookup".

#include <assert.h>
#include <stdbool.h>
#include "libradian.h"


// Update make_node() below if you alter any of these values.
#define NODE_SLOT_COUNT 6
#define NODE_KEY_SLOT 0
#define NODE_VALUE_SLOT 1
#define NODE_LEVEL_SLOT 2
#define NODE_LEFT_SLOT 3
#define NODE_RIGHT_SLOT 4
#define NODE_SIZE_SLOT 5

// Slots for node iterators
#define NODERATOR_SLOT_COUNT 2
#define NODERATOR_TARGET_SLOT 0
#define NODERATOR_PARENT_SLOT 1



static value_t Map_function( PREFUNC, value_t parameter );
static value_t Maperator_done_function( PREFUNC, value_t parameter );
static value_t Maperator_function( PREFUNC, value_t selector );

static struct closure Maperator_done = {(function_t)Maperator_done_function};

static value_t get_key( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function);
	if (node == &map_blank) return NULL;
	return node->slots[NODE_KEY_SLOT];
}

static value_t get_value( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function );
	if (node == &map_blank) return NULL;
	return node->slots[NODE_VALUE_SLOT];
}

static value_t get_level( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function );
	if (node == &map_blank) {
		return num_zero;
	}
	return node->slots[NODE_LEVEL_SLOT];
}

static value_t get_left( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function );
	if (node == &map_blank) return &map_blank;
	return node->slots[NODE_LEFT_SLOT];
}

static value_t get_right( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function );
	if (node == &map_blank) return &map_blank;
	return node->slots[NODE_RIGHT_SLOT];
}

static value_t get_size( zone_t zone, value_t node )
{
	if (IsAnException( node )) return node;
	assert( node && node->function == (function_t)Map_function );
	if (node == &map_blank) return num_zero;
	return node->slots[NODE_SIZE_SLOT];
}

static value_t make_node(
	zone_t zone,
	value_t key,
	value_t value,
	value_t level,
	value_t left,
	value_t right )
{
	struct closure *out = ALLOC( Map_function, NODE_SLOT_COUNT );
	out->slots[NODE_KEY_SLOT] = key;
	out->slots[NODE_VALUE_SLOT] = value;
	out->slots[NODE_LEVEL_SLOT] = level;
	out->slots[NODE_LEFT_SLOT] = left;
	out->slots[NODE_RIGHT_SLOT] = right;

	// The size equals the number of key-value pairs under this node, which is
	// the number on the left plus the number on the right plus one for self.
	value_t size = num_one;
	size = METHOD_1( size, sym_add, get_size( zone, left ) );
	size = METHOD_1( size, sym_add, get_size( zone, right ) );
	out->slots[NODE_SIZE_SLOT] = size;

	return out;
}

static value_t set_value( zone_t zone, value_t node, value_t new_value )
{
	return make_node(
		zone,
		get_key( zone, node ),
		new_value,
		get_level( zone, node ),
		get_left( zone, node ),
		get_right( zone, node ));
}

static value_t set_level( zone_t zone, value_t node, value_t new_level )
{
	return make_node(
		zone,
		get_key( zone, node ),
		get_value( zone, node ),
		new_level,
		get_left( zone, node ),
		get_right( zone, node ));
}

static value_t set_left( zone_t zone, value_t node, value_t new_left )
{
	return make_node(
		zone,
		get_key( zone, node ),
		get_value( zone, node ),
		get_level( zone, node ),
		new_left,
		get_right( zone, node ));
}

static value_t set_right( zone_t zone, value_t node, value_t new_right )
{
	return make_node(
		zone,
		get_key( zone, node ),
		get_value( zone, node ),
		get_level( zone, node ),
		get_left( zone, node ),
		new_right );
}

#if 0
// disabled because we don't always want to include stdio.h
#include <stdio.h>
#include "strings.h"

static void print_obj( value_t node )
{
	if (IsASymbol( node )) {
		fprintf( stderr, "%s", CStrFromSymbol( node ) );
	} else if (IsAStringLiteral( node )) {
		fprintf( stderr, "%s", CStrFromStringLiteral( node ) );
	} else if (IsAFixint( node )) {
		fprintf( stderr, "%d", IntFromFixint( node ) );
	} else {
		fprintf( stderr, "?" );
	}
}

static void print_tree( zone_t zone, value_t node, unsigned int indent )
{
	// debugging function for a specific type of tree with symbol keys and
	// string values
	for (unsigned int i = 0; i < indent; i++) fprintf( stderr, ".\t" );

	if (node == &map_blank) {
		fprintf( stderr, "*\n" );
		return;
	}

	print_obj( get_key( zone, node ) );
	fprintf( stderr, " => " );
	print_obj( get_value( zone, node ) );
	int level = IntFromFixint( get_level( zone, node ) );
	fprintf( stderr, " (%d)\n", level );

	print_tree( zone, get_left( zone, node ), indent + 1 );
	print_tree( zone, get_right( zone, node ), indent + 1 );
}
#endif


static value_t Skew( zone_t zone, value_t node )
{
	if (node == &map_blank) return node;
	// Maintain the AA-tree's right-leaning invariant.
	value_t level = get_level( zone, node );
	value_t left = get_left( zone, node );
	value_t left_level = get_level( zone, left );
	if (IntFromFixint( level ) == IntFromFixint( left_level )) {
		node = set_left( zone, node, get_right( zone, left ) );
		left = set_right( zone, left, node );
		node = left;
	}
	return node;
}

static value_t Split( zone_t zone, value_t node )
{
	if (node == &map_blank) return node;

	value_t level = get_level( zone, node );
	value_t right = get_right( zone, node );
	value_t right_right = get_right( zone, right );
	value_t right_right_level = get_level( zone, right_right );
	if (IntFromFixint( level ) == IntFromFixint( right_right_level )) {
		node = set_right( zone, node, get_left( zone, right ) );
		right = set_left( zone, right, node );
		node = right;
		value_t new_level = METHOD_1( level, sym_add, num_one );
		node = set_level( zone, node, new_level );
	}
	return node;
}

static value_t Insert(
	zone_t zone,
	value_t node,
	value_t key,
	value_t value )
{
	// Attempt to insert a new node in this subsection of the tree.
	// We will return the new node that stands for this section, and each call
	// will skew & split on its way back up in order to keep the tree balanced.

	if (node == &map_blank) {
		// We have reached the end of a branch: the new node will go here in
		// place of this map_blank. Its sub-branches will be the map_blank again,
		// which lets us repeat the trick next time we need to insert.
		return make_node(
				zone,
				key,
				value,
				num_one,
				&map_blank,
				&map_blank );
	}

	// Compare the key to the key of this node. We will get back an ordering
	// multiplexer which we can use to drill further into the data structure.
	value_t comparison = METHOD_1( key, sym_compare_to, get_key( zone, node ) );
	if (IsAnException( comparison )) return comparison;

	int relation = IntFromRelation( zone, comparison );
	if (relation < 0) {
		// The key to insert is ordered before the key at this node. We should
		// insert this pair onto the left branch.
		value_t newleft = Insert( zone, get_left( zone, node), key, value );
		node = set_left( zone, node, newleft );
	}
	else if (relation > 0) {
		// The key to insert is ordered after the key at this node. We should
		// insert this pair on the right branch.
		value_t newright = Insert( zone, get_right( zone, node ), key, value );
		node = set_right( zone, node, newright );
	}
	else {
		// The key to insert equals the key for this node. We will replace the
		// value for this node and leave the rest of the tree alone.
		// It would also be reasonable to raise an exception here, but I think
		// that most of the time that would be annoying. You can always make a
		// wrapper which checks first and raises an exception, if that's the
		// behaviour you prefer.
		return set_value( zone, node, value );
	}

	// rebalance the tree; should be right-leaning.
	// so refreshing to have such a simple rebalancing algorithm...
	value_t out = Split( zone, Skew( zone, node ) );
	return out;
}

static value_t Map_Insert(
		PREFUNC, value_t map_obj, value_t key, value_t value )
{
	ARGCHECK_3( map_obj, key, value );
	// Map method that implements the indexed-container interface. Allows you
	// to add a value at a specific index, aka key. This is the same as the
	// add function except it breaks the key and value out individually, which
	// makes it act like the array's version of the same method.
	return Insert( zone, map_obj, key, value );
}

static value_t Remove( zone_t zone, value_t node, value_t key )
{
	if (node == &map_blank) {
		// Dead end. We did not find the object we were looking for. No change
		// to the tree; return it as-is. We could possibly raise an exception
		// here instead, but that's a little more uptight than I really want
		// to be. You can always make a map wrapper which does the check and
		// raises an exception if you want that kind of safety.
		return node;
	}

	// Compare the key to the key of this node. We will get back an ordering
	// multiplexer which we can use to drill further into the data structure.
	value_t comparison = METHOD_1( key, sym_compare_to, get_key( zone, node ) );
	if (IsAnException( comparison )) return comparison;
	int relation = IntFromRelation( zone, comparison );

	if (relation < 0) {
		// Look down the left branch
		value_t newleft = Remove( zone, get_left( zone, node ), key );
		node = set_left( zone, node, newleft );
	}
	else if (relation > 0) {
		// Look down the right branch
		value_t newright = Remove( zone, get_right( zone, node ), key );
		node = set_right( zone, node, newright );
	}
	else {
		// This is the node we actually wanted to delete. If it has only one
		// branch, we return that branch in place of the deleted node; if it
		// has no branches, we return the map_blank. If it has two branches, we
		// will look down the right branch for the the deleted node's immediate
		// successor, remove it from the branch, and then re-establish it in
		// place of the deleted node.
		if (get_left( zone, node ) == &map_blank) {
			node = get_right( zone, node );
		}
		else if (get_right( zone, node ) == &map_blank) {
			node = get_left( zone, node );
		}
		else {
			// Find the deleted node's immediate successor. This will be the
			// leftmost node on the deleted node's right branch.
			value_t right = get_right( zone, node );
			value_t successor = right;
			while (get_left( zone, successor ) != &map_blank) {
				successor = get_left( zone, successor );
			}
			// Save the successor's key and value, then delete it from the
			// right branch, creating a new right-branch.
			value_t succ_key = get_key( zone, successor );
			value_t succ_val = get_value( zone, successor );
			right = Remove( zone, right, succ_key );
			// Re-create the successor node in place of the deleted node, using
			// the existing left branch and the newly modified right branch.
			node = make_node( zone, succ_key, succ_val, get_level( zone, node ),
				get_left( zone, node ), right );
		}
	}

	int threshold = IntFromFixint( get_level( zone, node ) ) - 1;
	value_t left = get_left( zone, node );
	value_t right = get_right( zone, node );
	if (IntFromFixint( get_level( zone, left ) ) < threshold  ||
		IntFromFixint( get_level( zone, right ) ) < threshold) {
		// If we have created a level gap, we need to rebalance the tree.
		// Reduce the level of the current node, since it has no sub-nodes that
		// would justify its current level.
		node = set_level( zone, node, NumberFromInt( zone, threshold ) );

		// If reducing the level of the current node put the right node above
		// the current one, reduce the right node too. This creates a
		// horizontal link, which we will rebalance in a moment.
		if (IntFromFixint( get_level( zone, right ) ) > threshold) {
			right = set_level( zone, right, NumberFromInt( zone, threshold ) );
			node = set_right( zone, node, right );
		}

		node = Skew( zone, node );

		right = get_right( zone, node );
		right = Skew( zone, right );
		node = set_right( zone, node, right );

		value_t right_right = get_right( zone, right );
		right_right = Skew( zone, right_right );
		right = set_right( zone, right, right_right );
		node = set_right( zone, node, right );

		node = Split( zone, node );

		right = get_right( zone, node );
		right = Split( zone, right );
		node = set_right( zone, node, right );
	}

	return node;
}

static value_t Map_Remove( PREFUNC, value_t map_obj, value_t key )
{
	ARGCHECK_2( map_obj, key );
	// Externally visible remove method for the map object
	return Remove( zone, map_obj, key );
}

static value_t lookup(
		zone_t zone, value_t node, value_t key, value_t default_func )
{
	// If we have reached a map_blank node, then we have failed to locate the
	// desired key, which means it is not present. We will report an error.
	if (node == &map_blank) {
		return CALL_1( default_func, key );
	}

	// Compare the key to the key of this node. We will get back an ordering
	// multiplexer which we can use to drill further into the data structure.
	value_t comparison =
			METHOD_1( key, sym_compare_to, get_key( zone, node ) );
	if (IsAnException( comparison )) return comparison;
	int compare = IntFromRelation( zone, comparison );

	if (compare < 0) {
		// The node we're looking for might live down the left branch.
		return lookup( zone, get_left( zone, node ), key, default_func );
	}
	if (compare > 0) {
		// The node we seek might live down the right branch.
		return lookup( zone, get_right( zone, node ), key, default_func );
	}
	// Neither branch? We must have found it!
	return get_value( zone, node );
}

static value_t Default_Lookup( PREFUNC, value_t key )
{
	ARGCHECK_1( key );
	value_t message = StringFromCStr( zone, "key not found" );
	return Throw( zone, AllocPair( zone, message, key ) );
}

static value_t Map_Lookup( PREFUNC, value_t map_obj, value_t key )
{
	ARGCHECK_2( map_obj, key );
	static struct closure default_result = {(function_t)Default_Lookup};
	return lookup( zone, map_obj, key, (value_t)&default_result );
}

static value_t Maperator_done_function( PREFUNC, value_t parameter )
{
	ARGCHECK_1( parameter );
	if (parameter == sym_current) {
		return ThrowCStr( zone, "the iterator is not valid" );
	}
	if (parameter == sym_next) {
		return ThrowCStr( zone, "the iterator is not valid" );
	}
	if (parameter == sym_is_valid) return &False_returner;
	return ThrowCStr( zone, "the map iterator does not have that method");
}

static value_t Maperator_Dig( zone_t zone, value_t node, value_t parent )
{
	if (node == &map_blank) return parent;
	// Dig all the way in on the left branch, returning an iterator for the
	// very leftmost node.
	struct closure *iter = ALLOC( Maperator_function, NODERATOR_SLOT_COUNT );
	iter->slots[NODERATOR_TARGET_SLOT] = node;
	iter->slots[NODERATOR_PARENT_SLOT] = parent;
	return Maperator_Dig( zone, node->slots[NODE_LEFT_SLOT], iter);
}

static value_t Maperator_Next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	// We are already dug in as far to the left as we can go. If there is a
	// right branch, dig into it for our next item, with the parent pointed
	// back not at us but at our parent. Otherwise, return our parent.
	value_t target = iterator->slots[NODERATOR_TARGET_SLOT];
	value_t parent = iterator->slots[NODERATOR_PARENT_SLOT];
	if (target->slots[NODE_RIGHT_SLOT]) {
		return Maperator_Dig( zone, target->slots[NODE_RIGHT_SLOT], parent );
	} else {
		return parent;
	}
}

static value_t Maperator_Current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	// This is not quite right; we need some way to detect 'set' style nodes,
	// where we should only return the key and not a (key,value) tuple. At
	// present we always return a tuple.
	value_t node = iterator->slots[NODERATOR_TARGET_SLOT];
	assert( node != &map_blank );
	value_t key = node->slots[NODE_KEY_SLOT];
	value_t value = node->slots[NODE_VALUE_SLOT];
	return AllocPair( zone, key, value );
}

static value_t Maperator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(next, Maperator_Next)
	DEFINE_METHOD(current, Maperator_Current)
	if (selector == sym_is_valid) return &True_returner;
	return ThrowCStr(
			zone, "the map iterator does not have the requested method");
}

static value_t contains( zone_t zone, value_t node, value_t key )
{
	if (node == &map_blank) {
		return BooleanFromBool( false );
	}
    value_t comparison =
            METHOD_1( key, sym_compare_to, get_key( zone, node ) );
    if (IsAnException( comparison )) return comparison;
    int compare = IntFromRelation( zone, comparison );
	if (compare < 0) {
		return contains( zone, get_left( zone, node ), key );
	}
	if (compare > 0) {
		return contains( zone, get_right( zone, node ), key );
	}
	return BooleanFromBool( true );
}

static value_t Map_Contains( PREFUNC, value_t map_obj, value_t key )
{
	ARGCHECK_2( map_obj, key );
	return contains( zone, map_obj, key );
}

static value_t Map_Iterate( PREFUNC, value_t map_obj )
{
	ARGCHECK_1( map_obj );
	// Perform a depth-first traversal of the tree. We will dig in all the way
	// on the left branch; it will be our first returned value.
	return Maperator_Dig( zone, map_obj, &Maperator_done );
}

static value_t Map_Size( PREFUNC, value_t map_obj )
{
	ARGCHECK_1( map_obj );
	return get_size( zone, map_obj );
}

static value_t Map_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(contains, Map_Contains)
	DEFINE_METHOD(iterate, Map_Iterate)
	if (sym_is_empty == selector) {
		return self == &map_blank ? &True_returner : &False_returner;
	}
	DEFINE_METHOD(insert, Map_Insert)
	DEFINE_METHOD(assign, Map_Insert)
	DEFINE_METHOD(remove, Map_Remove)
	DEFINE_METHOD(lookup, Map_Lookup)
	DEFINE_METHOD(size, Map_Size)
	return ThrowCStr( zone, "map does not implement that method" );
}

const struct closure map_blank = {(function_t)Map_function};
