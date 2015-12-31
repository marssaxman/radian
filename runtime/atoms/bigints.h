// Copyright 2013 Mars Saxman
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


#ifndef bigints_h
#define bigints_h

#include "../macros.h"
#include "../closures.h"
#include <stdint.h>

void init_bigints( zone_t zone );
bool IsABigint( value_t exp );
double DoubleFromBigint( value_t exp );
value_t BigintQuotient( zone_t zone, value_t numer, value_t denom );

// Don't do this generally: we assume that bigints are always bigger than
// fixints, that any number which can be represented by a fixint will be.
// We need some way of escaping to bigint land, though, so we can avoid
// overflow when an operation on fixints would produce a result out of fixint
// range. The fixint library will do this by creating a temporary bigint out
// of one of its operands, then calling some method on the bigint, returning
// whatever bigint result ends up being produced.
value_t MakeTemporaryBigint( zone_t zone, int value );

#if RUN_TESTS
void test_bigints( zone_t zone );
#endif

#endif
