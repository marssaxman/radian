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

#include "buffer.h"
#include <string.h>
#include "closures.h"
#include "exceptions.h"
#include "symbols.h"
#include "numbers.h"
#include "booleans.h"
#include "macros.h"

struct buffer *clone_buffer(
	zone_t zone,
	function_t function, 
	size_t bytes, 
	const void *data )
{
	size_t allocSize = sizeof(struct buffer) + bytes;
	struct buffer *out = 
		(struct buffer*)zone_alloc( zone, allocSize );
	memcpy( out->bytes, data, bytes );
	out->size = bytes;
	out->function = function;
	return out;
}

struct buffer *alloc_buffer( zone_t zone, function_t function, size_t bytes )
{
	size_t allocSize = sizeof(struct buffer) + bytes;
	struct buffer *out = 
		(struct buffer *)zone_alloc( zone, allocSize );
	// we probably should distinguish between fixed- and variable-size buffers;
	// fixed-size buffers don't need to store their byte count here
	out->size = bytes;
	out->function = function;
	return out;
}

static value_t Buffer_function( PREFUNC, value_t parameter );
static value_t Buffer_iterator_function( PREFUNC,
	value_t parameter );
static value_t Buffer_iterator_done_function( PREFUNC,
	value_t parameter );
static struct closure Buffer_iterator_done =
	{(function_t)Buffer_iterator_done_function};

static value_t Buffer_iterator_current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t buf = iterator->slots[0];
	int offset = IntFromFixint( iterator->slots[1] );
	return NumberFromInt( zone, BUFFER(buf)->bytes[offset] );
}

static value_t Buffer_iterator_next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t buf = iterator->slots[0];
	size_t offset = IntFromFixint( iterator->slots[1] );
	offset++;
	if (BUFFER(buf)->size > offset) {
		struct closure *out = ALLOC( Buffer_iterator_function, 2 );
		out->slots[0] = buf;
		out->slots[1] = NumberFromInt( zone, offset );
		return out;
	}
	else {
		return &Buffer_iterator_done;
	}
}

static value_t Buffer_iterator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(current, Buffer_iterator_current)
	DEFINE_METHOD(next, Buffer_iterator_next)
	if (sym_is_valid == selector) return &True_returner;
	return ThrowCStr( zone, "the buffer iterator does not have the requested method" );
}

static value_t Buffer_iterator_done_function( PREFUNC, value_t parameter )
{
	ARGCHECK_1( parameter );
	if (parameter == sym_current) return ThrowCStr( zone, "the iterator is not valid" );
	if (parameter == sym_next) return ThrowCStr( zone, "the iterator is not valid" );
	if (parameter == sym_is_valid) return &False_returner;
	return ThrowCStr( zone, "the buffer iterator object does not have that method" );
}

static value_t Buffer_iterate( PREFUNC, value_t buf )
{
	ARGCHECK_1( buf );
	if (BUFFER(self)->size) {
		struct closure *out = ALLOC( Buffer_iterator_function, 2 );
		out->slots[0] = buf;
		out->slots[1] = num_zero;
		return out;
	}
	else {
		return &Buffer_iterator_done;
	}
}

static value_t Buffer_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	// There's no technical reason the buffer could not have an index function
	// as well as the iterate function, but the whole point of Radian is to do
	// as much as possible in terms of sequences. I will leave the buffer as
	// nothing but a sequence for now and consider adding more features later
	// if appropriate.
	DEFINE_METHOD(iterate, Buffer_iterate)
	return ThrowCStr( zone, "buffer object does not have that method" );
}

struct buffer *Buffer( zone_t zone, size_t bytes )
{
	return BUFALLOC( Buffer_function, bytes );
}

