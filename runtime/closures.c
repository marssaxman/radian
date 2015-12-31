// Copyright 2009-2013 Mars Saxman
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


#include "closures.h"
#include "buffer.h"
#include "booleans.h"
#include "allocator.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"


struct closure *alloc_object( zone_t zone, function_t function, size_t slots )
{
	// Create a block large enough to hold the closure header and the
	// requested number of slots. The caller will populate the slots
	// before releasing the new block into the wild.
	size_t allocSize = sizeof(struct closure) + slots * sizeof(value_t);
	struct closure *out = (struct closure*)zone_alloc( zone, allocSize );
	out->function = function;
	return out;
}

static value_t is_not_void_func(PREFUNC, value_t param)
{
	return BooleanFromBool(param != NULL);
}
struct closure is_not_void = {(function_t)is_not_void_func};

value_t method_0( zone_t zone, value_t obj, value_t sym )
{
	value_t method = CALL_1( obj, sym );
	return CALL_1( method, obj );
}

value_t method_1( zone_t zone, value_t obj, value_t sym, value_t arg0 )
{
	value_t method = CALL_1( obj, sym );
	return CALL_2( method, obj, arg0 );
}

value_t method_2(
		zone_t zone, value_t obj, value_t sym, value_t arg0, value_t arg1 )
{
	value_t method = CALL_1( obj, sym );
	return CALL_3( method, obj, arg0, arg1 );
}
