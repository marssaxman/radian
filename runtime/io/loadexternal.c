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

#include "loadexternal.h"
#include "string.h"
#include <assert.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include "tuples.h"
#include "numbers.h"
#include "strings.h"
#include "exceptions.h"
#include "symbols.h"
#include "booleans.h"
#include "buffer.h"
#include "macros.h"

static value_t Pointer_function( PREFUNC,
	value_t parameter )
{
	if (IsAnException(parameter)) return parameter;
	return ThrowCStr( zone, "symbol does not have that member" );
}

bool IsAPointer( value_t exp )
{
	return exp && exp->function == (function_t)Pointer_function;
}

value_t IOAction_LoadExternal_2( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	value_t library_path_obj = action->slots[ IOACTION_SLOT_COUNT + 0 ];
	value_t function_name_obj = action->slots[ IOACTION_SLOT_COUNT + 1 ];
	
	value_t exception = NULL;
	const char *library_path = NULL;
	const char *function_name = NULL;
	
	library_path = UnpackString( zone, library_path_obj );
	if (!library_path) {
		exception = ThrowCStr( zone, "library path is not a string" );
		goto error;
	}
	function_name = UnpackString( zone, function_name_obj );
	if (!function_name) {
		exception = ThrowCStr( zone, "symbol name is not a string" );
		goto error;
	}

	void *library = dlopen( library_path, RTLD_NOW );
	if (!library) {
		exception = ThrowCStr( zone, "could not open the library" );
		goto error;
	}

	void *funcptr = dlsym( library, function_name );
	if (!funcptr) {
		exception = ThrowCStr( zone, "could not load the symbol" );
		goto error;
	}

	struct buffer *out = BUFALLOC( Pointer_function, sizeof(void*) );
	memcpy( out->bytes, &funcptr, sizeof(void*) );
	return (value_t)out;

error:
	if (library_path) free( (void*)library_path );
	if (function_name) free( (void*)function_name );
	return exception;
}

static value_t FFI_Load_External_function(
		PREFUNC, value_t lib, value_t name )
{
	ARGCHECK_2( lib, name );
	static struct closure proc = {(function_t)IOAction_LoadExternal_2};
	return MakeAsyncIOAction_2( zone, &proc, lib, name );
}
const struct closure FFI_Load_External =
		{(function_t)FFI_Load_External_function};
