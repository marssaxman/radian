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


#ifndef numbers_h
#define numbers_h

#include "../macros.h"
#include "../closures.h"
#include "fixints.h"
#include <stdbool.h>

// Convenient references to the two most commonly used numbers. Only valid
// after init_numbers() has executed. Do not ever change these, nor compare
// things to these references - numbers are not interned.
extern value_t num_zero;
extern value_t num_one;

void init_numbers( zone_t zone );
value_t NumberLiteral( zone_t zone, const char *data, size_t length );

bool IsANumber( value_t exp );
bool IsAnInteger( value_t exp );
value_t NaNExp( zone_t zone, value_t obj );
value_t DivByZeroExp( zone_t zone );

#if RUN_TESTS
void test_numbers( zone_t zone );
#endif

#endif // numbers_h
