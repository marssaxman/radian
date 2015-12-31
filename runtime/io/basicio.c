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


#include "basicio.h"
#include "ioaction.h"
#include "strings.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stringliterals.h"
#include "symbols.h"
#include "exceptions.h"
#include "numbers.h"
#include "booleans.h"
#include "buffer.h"
#include "macros.h"

value_t IOAction_Print_1( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	value_t string = action->slots[ IOACTION_SLOT_COUNT + 0 ];
	// a string is a sequence of characters. We will iterate through this
	// sequence and print each char to stdout.
	const char *cstr = UnpackString( zone, string );
	if (!cstr) {
		return ThrowCStr( zone, "print argument must be a string" );
	}
	fprintf( stderr, "%s\n", cstr );
	free( (void*)cstr );
	// Radian has no 'nil' or 'void' value. The result of a successful print is
	// nothing. We can't return an exception, though, because that means the
	// print failed, and will derail the task. This result isn't right, but it
	// is at least less wrong than returning an exception.
	return num_zero;
}

value_t IOAction_Input_0( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	// read a line from stdin; create a string, pass it to the argument
	// function, along with a new IO instance. The result will be another IO
	// queue. The argument is an IO continuation function. We will start with
	// some reasonably sized line buffer, expanding it as necessary until we
	// reach a linebreak. Then we will create a string from this buffer.
	size_t bufsiz = 80;
	char *buffer = malloc(bufsiz);
	size_t charcount = 0;
	while (true) {
		char ch = fgetc(stdin);
		if (ch == EOF || ch == '\n') break;
		if (charcount == bufsiz) {
			buffer = realloc(buffer, bufsiz *= 2);
			assert( buffer );
		}
		buffer[charcount++] = ch;
	}
	value_t newString = StringFromUTF8Bytes( zone, buffer, charcount );
	free( buffer );
	return newString;
}

static FILE *open_file( zone_t zone, value_t file, const char *mode )
{
	// We consider anything which offers a 'path' string to be a file
	// specification.
	if (IsAnException( file )) return NULL;
	const char *path = UnpackString( zone, file );
	if (!path) return NULL;
	FILE *out = fopen( path, mode );
	free( (void*)path );
	return out;
}

static value_t IOAction_ReadFile_1( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	value_t file = action->slots[ IOACTION_SLOT_COUNT + 0 ];
	FILE *fd = open_file( zone, file, "rb" );
	if (!fd) return ThrowCStr( zone, "failed to open file" );

	// Find out how large the file is, so we can allocate an appropriately-
	// sized buffer.
	fseek( fd, 0, SEEK_END );
	size_t length = ftell( fd );
	fseek( fd, 0, SEEK_SET );

	// create a buffer to contain the data we will read, then read in the
	// entire contents of the source file.
	struct buffer *data = Buffer( zone, length );
	fread( data->bytes, 1, length, fd );
	fclose( fd );
	return (value_t)data;
}

static value_t Read_File_function( PREFUNC, value_t path )
{
	ARGCHECK_1( path );
    static struct closure proc = {(function_t)IOAction_ReadFile_1};
    return MakeAsyncIOAction_1( zone, &proc, path );
}

const struct closure Read_File = {(function_t)Read_File_function};


static value_t IOAction_WriteFile_2( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	value_t file = action->slots[ IOACTION_SLOT_COUNT + 0 ];
	value_t contents = action->slots[ IOACTION_SLOT_COUNT + 1 ];

	FILE *fd = open_file( zone, file, "w" );
	if (!fd) return ThrowCStr( zone, "failed to open file" );

	value_t iter = METHOD_0( contents, sym_iterate );

	// Read bytes from the contents sequence until it ends. Write each byte to
	// the file. It might be nice to do something with buffers here, but we can
	// worry about that later.
	while (true) {
		if (IsAnException( iter )) return iter;
		value_t is_valid = METHOD_0( iter, sym_is_valid );
		if (IsAnException( is_valid )) {
			fclose( fd );
			return is_valid;
		}
		if (!BoolFromBoolean( zone, is_valid )) break;

		value_t current = METHOD_0( iter, sym_current );
		if (IsAnException( current )) {
			fclose( fd );
			return current;
		}

		if (!IsAFixint( current )) {
			fclose( fd );
			return ThrowCStr(
					zone, "output values must be numbers in the range 0..255" );
		}
		int byte = IntFromFixint( current );
		if (byte < 0 || byte > 255) {
			fclose( fd );
			return ThrowCStr(
					zone, "output values must be numbers in the range 0..255" );
		}

		// Write the actual byte to the actual file!
		fputc( byte, fd );

		// Now get the next iterator value so we can write the next byte. Wow
		// what a lot of overhead.
		iter = METHOD_0( iter, sym_next );
	}
	fclose( fd );
	// Return some meaningless non-exceptional value. Do we need a 'void'?
	// Maybe some IO-specific "success" value.
	return num_zero;
}

static value_t Write_File_function( PREFUNC, value_t path, value_t bytes )
{
	ARGCHECK_2( path, bytes );
    static struct closure proc = {(function_t)IOAction_WriteFile_2};
    return MakeAsyncIOAction_2( zone, &proc, path, bytes );
}

const struct closure Write_File = {(function_t)Write_File_function};


