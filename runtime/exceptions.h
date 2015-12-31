// Copyright 2010-2013 Mars Saxman
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

#ifndef exceptions_h
#define exceptions_h

#include "closures.h"
#include <stdbool.h>
#include <stdint.h>

bool IsAnException( value_t exp );
value_t Throw( zone_t zone, value_t value );
value_t ThrowCStr( zone_t zone, const char *msg );
value_t ThrowMemberNotFound( zone_t zone, value_t sym );
value_t ThrowArgCountFail(
		zone_t zone, const char *function, int32_t expected, int32_t actual );
value_t ThrowCStrNotFound( zone_t zone, const char *msg, value_t sym );
value_t ExceptionContents( value_t exception );

extern struct closure throw_exception;
extern struct closure catch_exception;
extern struct closure is_not_exceptional;

#endif	// exceptions_h
