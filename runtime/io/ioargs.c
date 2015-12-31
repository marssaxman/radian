// Copyright 2010-2012 Mars Saxman
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


#include "ioargs.h"
#include "numbers.h"
#include "symbols.h"
#include "exceptions.h"
#include "booleans.h"
#include "../atoms/stringliterals.h"
#include "macros.h"

static value_t Args_iter_function( PREFUNC, value_t sym );

static value_t Args_iter_valid( PREFUNC, value_t args )
{
	// The iterator is valid if there are any args to iterate over - that is,
	// the count is greater than zero
	int count = IntFromFixint( args->slots[0] );
	return BooleanFromBool( count > 0 );
}

static value_t Args_iter_current( PREFUNC, value_t args )
{
	// Which is the string we are currently looking at? It is always first in
	// the list.
	int count = IntFromFixint( args->slots[0] );
	if (count < 1) {
		return ThrowCStr( zone, "invalid iterator has no current value" );
	}
	return args->slots[1];
}

static value_t Args_iter_next( PREFUNC, value_t args )
{
	// Return the next iterator: reduce the count by one and copy all but the
	// first string.
	int count = IntFromFixint( args->slots[0] );
	if (count < 1) {
		return ThrowCStr( zone, "invalid iterator has no next iterator" );
	}
	struct closure *out = ALLOC( Args_iter_function, count - 1 + 1 );
	out->slots[0] = NumberFromInt( zone, count - 1 );
	for (int i = 1; i < count; i++) {
		out->slots[i] = args->slots[i + 1];
	}
	return out;
}

static value_t Args_iter_function( PREFUNC, value_t selector )
{
	if (IsAnException( selector )) return selector;
	DEFINE_METHOD( is_valid, Args_iter_valid );
	DEFINE_METHOD( current, Args_iter_current );
	DEFINE_METHOD( next, Args_iter_next );
	return ThrowCStr( zone, "The object does not have the requested method" );
}

static value_t Args_seq_iterate( PREFUNC, value_t args )
{
	int count = IntFromFixint( args->slots[0] );
	struct closure *out = ALLOC( Args_iter_function, count + 1 );
	out->slots[0] = NumberFromInt( zone, count );
	for (int i = 0; i < count; i++) {
		out->slots[i + 1] = args->slots[i + 1];
	}	
	return out;
}

static value_t Args_seq_function( PREFUNC, value_t selector )
{
	if (IsAnException( selector )) return selector;
	DEFINE_METHOD( iterate, Args_seq_iterate );
	return ThrowCStr( zone, "The object does not have the requested method" );
}

value_t Args( zone_t zone, int argc, char *const argv[] )
{
	// Create a sequence of arguments passed in to the program. This is useful
	// when writing apps designed to be run from a command line, which will
	// probably be most of the time.
	struct closure *out = ALLOC( Args_seq_function, argc + 1 );
	out->slots[0] = NumberFromInt( zone, argc );
	for (int i = 0; i < argc; i++) {
		out->slots[i + 1] = StringFromCStr( zone, argv[i] );
	}
	return out;
}

