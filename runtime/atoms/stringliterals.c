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


// The "string" interface is simply a sequence of Unicode characters.
// A string atom is an object which represents some literal value as a string.
// The compiler generates code which calls the string atom constructor, passing
// in a C-style null-terminated buffer of chars encoded in UTF-8. The string
// atom presents this buffer as a sequence, with the expected comparison and
// concatenation methods.


#include "stringliterals.h"
#include "buffer.h"
#include "numbers.h"
#include "booleans.h"
#include "exceptions.h"
#include "strings.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"
#include "macros.h"

#define STRING_ITERATOR_SLOT_COUNT 2
#define STRING_ITERATOR_TARGET_SLOT 0
#define STRING_ITERATOR_OFFSET_SLOT 1

static value_t String_function( PREFUNC, value_t parameter );
static value_t String_iterator_function( PREFUNC, value_t parameter );
static value_t String_iterator_done_function( PREFUNC, value_t parameter );
static struct closure String_iterator_done =
		{(function_t)String_iterator_done_function};

static value_t String_iterator_current( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t string = iterator->slots[STRING_ITERATOR_TARGET_SLOT];
	unsigned offset =
			IntFromFixint( iterator->slots[STRING_ITERATOR_OFFSET_SLOT] );
	// Decode the UTF-8 character located at this position. If we find any
	// problems, we will return U+FFFD, the standard error character.

	// Make sure we have not yet run off the end of the buffer.
	if (offset >= BUFFER(string)->size) {
		return NumberFromInt( zone, 0xFFFD );
	}

	// Peek at the first byte of the char to figure out how long the whole
	// production is going to be.
	unsigned char *src = &(BUFFER(string)->bytes[offset]);
	unsigned char ch = *src;
	unsigned int charlen = 1;
	if (0xF0 == (ch & 0xF0)) {
		charlen = 4;
	} else if (0xE0 == (ch & 0xE0)) {
		charlen = 3;
	} else if (0xC0 == (ch & 0xC0)) {
		charlen = 2;
	}

	// Some bytes begin character sequences and others continue them. We must
	// never start with a continuing character.
	if (0x80 == (ch & 0xC0)) {
		return NumberFromInt( zone, 0xFFFD );
	}

	// Make sure there is enough space left in the buffer to hold this char.
	if (offset + charlen > BUFFER(string)->size) {
		return NumberFromInt( zone, 0xFFFD );
	}

	// We've cleared away all the possible errors and we know how many bytes
	// will contribute to the char. Assemble it and return it.
	unsigned int out = 0;
	switch (charlen) {
		case 1: {
			out = *src;
		} break;
		case 2: {
			out = ((src[0] & 0x1F) << 6) |
				(src[1] & 0x3F );
		} break;
		case 3: {
			out = ((src[0] & 0x0F) << 12) |
				(src[1] & 0x3F) << 6 |
				(src[2] & 0x3F);
		} break;
		case 4: {
			out = ((src[0] & 0x07) << 18) |
				(src[1] & 0x3F) << 12 |
				(src[2] & 0x3F) << 6 |
				(src[3] & 0x3F);
		} break;
	}
	return NumberFromInt( zone, out );
}

static value_t String_iterator_next( PREFUNC, value_t iterator )
{
	ARGCHECK_1( iterator );
	value_t string = iterator->slots[STRING_ITERATOR_TARGET_SLOT];
	unsigned offset =
			IntFromFixint( iterator->slots[STRING_ITERATOR_OFFSET_SLOT] );

	// Advance past the current character so we can prepare to read in the next
	// one. The leading byte of a UTF-8 character encodes the length of the
	// char's represention.
	char ch = BUFFER(string)->bytes[offset];
	if (0xF0 == (ch & 0xF0)) {
		offset += 4;
	} else if (0xE0 == (ch & 0xE0)) {
		offset += 3;
	} else if (0xC0 == (ch & 0xC0)) {
		offset += 2;
	} else {
		offset += 1;
	}

	// If we have run off the end of the string, bail out and tell the caller
	// that our string is finished. There are two ways this might happen: we
	// might have run out of bytes in the buffer, or we might have encountered
	// a NULL character. The latter only exists because we leave stray null
	// bytes in our buffers - this lets CStrFromStringLiteral return a pointer
	// to the string buffer without having to do a copy.
	if (offset >= BUFFER(string)->size) {
		return &String_iterator_done;
	}
	if ('\0' == BUFFER(string)->bytes[offset]) {
		return &String_iterator_done;
	}

	// Create an iterator pointing at the next character in the string.
	struct closure *out =
			ALLOC( String_iterator_function, STRING_ITERATOR_SLOT_COUNT );
	out->slots[STRING_ITERATOR_TARGET_SLOT] = string;
	out->slots[STRING_ITERATOR_OFFSET_SLOT] = NumberFromInt( zone, offset );
	return out;
}

static value_t String_iterator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(current, String_iterator_current)
	DEFINE_METHOD(next, String_iterator_next)
	if (selector == sym_is_valid) return &True_returner;
	return ThrowCStrNotFound( zone, "not found (string iterator)", selector );
}

static value_t String_iterator_done_current( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return ThrowCStr( zone, "the iterator is not valid" );
}

static value_t String_iterator_done_next( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return ThrowCStr( zone, "the iterator is not valid" );
}

static value_t String_iterator_done_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(current, String_iterator_done_current)
	DEFINE_METHOD(next, String_iterator_done_next)
	if (selector == sym_is_valid) return &False_returner;
	return ThrowCStrNotFound( zone, "not found (string iter done)", selector );
}

static value_t String_iterate( PREFUNC, value_t string )
{
	ARGCHECK_1( string );
	if (BUFFER(string)->bytes[0]) {
		struct closure *out =
				ALLOC( String_iterator_function, STRING_ITERATOR_SLOT_COUNT );
		out->slots[STRING_ITERATOR_TARGET_SLOT] = string;
		out->slots[STRING_ITERATOR_OFFSET_SLOT] = num_zero;
		return out;
	}
	else {
		return &String_iterator_done;
	}
}

static value_t String_concatenate( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	return ConcatStrings( zone, left, right );
}

static value_t String_compare_to( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	return CompareStrings( zone, left, right );
}

static value_t String_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(iterate, String_iterate)
	DEFINE_METHOD(concatenate, String_concatenate)
	DEFINE_METHOD(compare_to, String_compare_to)
	return ThrowCStrNotFound( zone, "not found (string)", selector );
}

value_t StringLiteral( zone_t zone, const char *data, size_t length )
{
	// The compiler bakes string literals in as zero-terminated C strings.
	// We don't need to allocate a new buffer for these, because we can assume
	// that their storage will never be freed, or need to be freed.
	// We will allocate such a buffer anyway, though, since I haven't yet done
	// the work necessary to leave it unallocated: buffers always own their
	// storage.
	return StringFromUTF8Bytes( zone, data, length );
}

value_t StringFromCStr( zone_t zone, const char *data )
{
	// data is a pointer to a zero-terminated C string in UTF-8 encoding.
	// We cannot assume the storage is persistent, so we must copy the bytes
	// into a new buffer which we will manage.
	size_t bytes = strlen( data );
	return StringFromUTF8Bytes( zone, data, bytes );
}

value_t StringFromUTF8Bytes( zone_t zone, const char *data, size_t bytes )
{
	// data is a pointer to some bytes which are UTF-8 encoded chars
	// length is the number of bytes involved
	// we cannot use clone_buffer(), because it wants to copy an entire buffer,
	// whereas we need to insert an additional terminator byte so that the
	// contents of a string literal atom can function as a C string.
	struct buffer *out =
			alloc_buffer( zone, (function_t)String_function, bytes + 1 );
	memcpy( out->bytes, data, bytes );
	out->bytes[bytes] = '\0';
	return (value_t)out;
}

value_t ConcatStringLiterals( zone_t zone, value_t left, value_t right )
{
	assert( IsAStringLiteral( left ) );
	assert( IsAStringLiteral( right ) );
	// We know that each of these has a buffer which includes a terminator.
	// We will create a new buffer large enough to contain both buffers but
	// only one terminator.
	size_t left_size = BUFFER(left)->size - 1;
	size_t right_size = BUFFER(right)->size - 1;
	size_t new_size = left_size + right_size + 1;
	struct buffer *out =
			alloc_buffer( zone, (function_t)String_function, new_size );
	memcpy( out->bytes, BUFFER(left)->bytes, left_size );
	memcpy( &out->bytes[left_size], BUFFER(right)->bytes, right_size );
	out->bytes[new_size-1] = '\0';
	return (value_t)out;
}

// String_From_Codepoint
//
// Internal function, available only to the standard library.
// Creates a one-character string literal using the codepoint provided. The
// codepoint must be an integer; we assume we know what the internal format of
// an integer object is.
//
static value_t String_From_Codepoint( PREFUNC, value_t number )
{
	ARGCHECK_1( number );
	if (!IsAFixint( number )) {
		return ThrowCStr( zone, "operand must be an integer" );
	}
	int ch = *BUFDATA( number, int );
	// If char is out of the unicode range, we will encode it as U+FFFD, which
	// is the standard Unicode error signal.
	if (ch < 0 || ch > 0x10FFFF) {
		ch = 0xFFFD;
	}
	char bytes[4];
	int len = 0;
	if (ch <= 0x00007F) {
		bytes[len++] = ch;
	} else if (ch >= 0x000080 && ch <= 0x0007FF) {
		bytes[len++] = 0xC0 | ((ch >> 6) & 0x1F);
		bytes[len++] = 0x80 | (ch & 0x3F);
	} else if (ch >= 0x000800 && ch <= 0x00FFFF) {
		bytes[len++] = 0xE0 | ((ch >> 12) & 0x0F);
		bytes[len++] = 0x80 | ((ch >> 6) & 0x3F);
		bytes[len++] = 0x80 | (ch & 0x3F);
	} else if (ch >= 0x010000 && ch <= 0x10FFFF) {
		bytes[len++] = 0xF0 | ((ch >> 18) & 0x0F);
		bytes[len++] = 0x80 | ((ch >> 12) & 0x3F);
		bytes[len++] = 0x80 | ((ch >> 6) & 0x3F);
		bytes[len++] = 0x80 | (ch & 0x3F);
	}
	return StringFromUTF8Bytes( zone, bytes, len );
}
const struct closure char_from_int = {(function_t)String_From_Codepoint};

const char *CStrFromStringLiteral( value_t it )
{
	assert( IsAStringLiteral( it ) );
	return (const char*)BUFFER(it)->bytes;
}

bool IsAStringLiteral( value_t it )
{
	return it && it->function == (function_t)String_function;
}
