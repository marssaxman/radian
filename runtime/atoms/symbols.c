// Copyright 2009-2014 Mars Saxman
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
#include "libradian.h"
#include "symbols.h"
#include "exceptions.h"
#include "relations.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "threads.h"

// The symbol struct is the data payload in a buffer object. This means we have
// object pointers in a buffer, which is normally not allowed due to garbage
// collection constraints. We get away with it because these refs only ever
// point at other symbols, and symbols are immortal.
struct symbol {
	const char *key;
	unsigned int level;
	value_t left;
	value_t right;
};

// Insert lock protects writes to the symbol tree.
static thread_mutex_t sInsertLock;
static value_t sSymbolTree;

// Define the storage for all of the symbol variables.
#define SYMBOL(x) value_t sym_##x
#include "symbol-list.h"
#undef SYMBOL

static value_t Symbol_compare_to( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (!IsASymbol( left ) || !IsASymbol( right )) {
		return ThrowCStr( zone, "can't compare a symbol with a non-symbol");
	}
	const char *leftkey = BUFDATA(left, struct symbol)->key;
	const char *rightkey = BUFDATA(right, struct symbol)->key;
	int relation = strcmp( leftkey, rightkey );
	if (relation < 0) return LessThan();
	else if (relation > 0) return GreaterThan();
	else return EqualTo();
}

static value_t Symbol_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(compare_to, Symbol_compare_to)
	return ThrowCStr( zone, "symbol does not have that member");
}

static bool Insert( value_t *target, const char *data, value_t *dest )
{
	// Attempt to insert a new symbol representing the char data into the
	// master symbol tree. We maintain a balanced tree using the Andersson
	// algorithm; that is, this is an AA-tree like the standard map container.
	assert( target && data && dest );
	value_t node = *target;

	if (NULL == node) {
		// End of the line: we have found the spot the symbol should live, and
		// there is nothing there. We will fill it with a new symbol instance.
		// Symbols must live in the root zone, because they are global and
		// immortal, shared among all threads; they must never be collected.
		zone_t zone = root_zone();
		struct buffer *out = BUFALLOC( Symbol_function, sizeof(struct symbol) );
		BUFDATA(out, struct symbol)->key = data;
		BUFDATA(out, struct symbol)->level = 1;
		*target = (value_t)out;
		*dest = (value_t)out;
		return true;
	}

	const char *nodekey = BUFDATA(node, struct symbol)->key;
	int relation = strcmp( nodekey, data );
	bool did_modify = false;
	if (relation > 0) {
		did_modify = Insert( &BUFDATA(node, struct symbol)->left, data, dest );
	} else if (relation < 0) {
		did_modify = Insert( &BUFDATA(node, struct symbol)->right, data, dest );
	} else {
		// The quick search does not assert the mutex, so it is vulnerable to
		// false negatives. Some other thread was modifying the tree while we
		// waited for the mutex: either it just inserted the same symbol we are
		// now looking for, or it was rebalancing the tree in a way that left
		// the target symbol inaccessible. Either way, we have found the symbol
		// now, so we might as well return it.
		*dest = node;
	}

	// If one of our children inserted a new node, we need to rebalance the
	// tree. First we will perform the "skew": if our left node's level is
	// equal to our level, rotate right.
	unsigned int level = BUFDATA(node, struct symbol)->level;
	value_t left = BUFDATA(node, struct symbol)->left;
	value_t right = BUFDATA(node, struct symbol)->right;
	if (did_modify && left && level == BUFDATA(left, struct symbol)->level) {
		BUFDATA(node, struct symbol)->left = 
			BUFDATA(left, struct symbol)->right;
		BUFDATA(left, struct symbol)->right = node;
		node = left;
	}

	// Next perform the "split": if our right node's right node is equal to
	// our level, rotate left. This may raise us up a level.
	if (did_modify && right) {
		value_t rightright = BUFDATA(right, struct symbol)->right;
		if (rightright && BUFDATA(rightright, struct symbol)->level == level) {
			BUFDATA(node, struct symbol)->right =
				BUFDATA(right, struct symbol)->left;
			BUFDATA(right, struct symbol)->left = node;
			node = right;
			BUFDATA(node, struct symbol)->level++;
		}
	}

	if (did_modify) {
		*target = node;
	}

	return did_modify;
}

value_t SymbolLiteral( zone_t zone, const char *data )
{
	// We don't actually use the caller's zone. Symbols are idempotent, so
	// we store them in the global zone. They must never be collected. We only
	// get a reference to the caller's zone because that's the protocol for
	// atom constructors.
	// We expect that the data is immortal - this function is designed for
	// symbol literals, so the input data should come from the executable's
	// static data section.

	// Perform a quick, optimistic, nonrecursive search. Most of the time we
	// ought to find the symbol we are looking for, so we will not waste a lot
	// of time dealing with mutexes.
	value_t target = sSymbolTree;
	while (target) {
		assert( IsASymbol( target ) );
		const char *targetkey = BUFDATA(target, struct symbol)->key;
		int relation = strcmp( targetkey, data );
		if (0 == relation) {
			return target;
		} else if (relation > 0) {
			target = BUFDATA(target, struct symbol)->left;
		} else if (relation < 0) {
			target = BUFDATA(target, struct symbol)->right;
		}
	}

	// We didn't find a match, so we'll try to insert a new symbol instance.
	// For this operation, we definitely need exclusive write access to the
	// symbol tree.
	thread_mutex_lock( &sInsertLock );
	value_t out = NULL;
	Insert( &sSymbolTree, data, &out );
	thread_mutex_unlock( &sInsertLock );
	return out;
}

bool IsASymbol( value_t obj )
{
	return obj && obj->function == (function_t)Symbol_function;
}

const char *CStrFromSymbol( value_t it )
{
	assert( IsASymbol( it ) );
	return BUFDATA(it, struct symbol)->key;
}

void init_symbols( zone_t zone )
{
	thread_mutex_create( &sInsertLock );
	// Initialize all of the symbol variables.
	#define SYMBOL(x) sym_##x = SymbolLiteral( zone, #x )
	#include "symbol-list.h"
	#undef SYMBOL
}

