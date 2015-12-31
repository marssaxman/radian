// Copyright 2012 Mars Saxman
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


#ifndef strings_h
#define strings_h

#include "closures.h"

value_t ConcatStrings( zone_t zone, value_t left, value_t right );
value_t CompareStrings( zone_t zone, value_t left, value_t right );

// Unpack a sequence of Unicode characters into a zero-terminated, C-style
// string using UTF-8 encoding. It will malloc a buffer, so you must make sure
// to free() it when you are done. After this operation is complete, the buffer
// will have no connection to the original string object.
// The parameter must be a sequence of codepoints: it doesn't actually have to
// match the whole string protocol (including compare_to and concatenate).
// In the error case, this will return NULL; in the case of an empty string, it
// will return a minimally sized buffer whose zero'th byte is zero.
const char *UnpackString( zone_t zone, value_t it );

#endif //strings_h
