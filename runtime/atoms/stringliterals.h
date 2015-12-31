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


#ifndef stringliterals_h
#define stringliterals_h

#include "../closures.h"
#include <stdbool.h>

// Compiler-invoked entrypoint to generate string literal objects
value_t StringLiteral( zone_t zone, const char *data, size_t length );

// Builtin entrypoint so library can synthesize strings from numbers
extern const struct closure char_from_int;

// Runtime utility functions for creating strings from C
value_t StringFromCStr( zone_t zone, const char *data );
value_t StringFromUTF8Bytes( zone_t zone, const char *data, size_t bytes );

// CStrFromStringLiteral will peek at a string literal object's buffer. This is
// almost certainly not the tool you want to use, since it will not work for
// any string which is not a string literal. Look at io/unpackstring.h instead.
// It is almost certainly not safe to use this function on a value which has
// come back from compiled code; only use it when you need to print debugging
// output and you're certain that you created the string involved using one of
// the above StringFrom functions.
const char *CStrFromStringLiteral( value_t it );
bool IsAStringLiteral( value_t it );

// Make a new string literal object from two existing ones, by concatenating
// their buffers. This is almost certainly not the function you want; look in
// strings.h for the general ConcatStrings() function instead.
value_t ConcatStringLiterals( zone_t zone, value_t left, value_t right );

#endif //stringliterals_h
