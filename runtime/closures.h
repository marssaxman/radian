// Copyright 2009-2011 Mars Saxman
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


#ifndef closures_h
#define closures_h
#include <stddef.h>
#include "memory/allocator.h"

typedef unsigned char byte_t;

// An invokable is a function with some captured data.
// The data may be either a buffer or a list of object pointers ("slots").
// An invokable's backing function expects three implicit parameters, supplying
// the current alloc zone, the invokable itself (for access to the captured
// data), and the number of args (for validation).
// Invokables are closures by default, because most invokables are not buffers.
typedef const struct closure *value_t;
#define PREFUNC zone_t zone, value_t self, int argc
typedef value_t (*function_t)(PREFUNC, ...);

struct closure
{
	function_t function;
	value_t slots[];
};

struct closure *alloc_object( zone_t zone, function_t function, size_t slots );

extern struct closure is_not_void;

value_t method_0( zone_t zone, value_t obj, value_t sym );
value_t method_1( zone_t zone, value_t obj, value_t sym, value_t arg0 );
value_t method_2(
		zone_t zone, value_t obj, value_t sym, value_t arg0, value_t arg1 );

#endif //closures_h

