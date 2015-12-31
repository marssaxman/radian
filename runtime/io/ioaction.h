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

#ifndef ioaction_h
#define ioaction_h

#include <stdbool.h>
#include "../closures.h"

// By convention, each IO action function will include a suffix denoting the
// number of arguments it expects. Make sure the action maker matches.

value_t MakeAsyncIOAction_0( zone_t zone, value_t target );
value_t MakeAsyncIOAction_1( zone_t zone, value_t target, value_t arg0 );
value_t MakeAsyncIOAction_2(
		zone_t zone, value_t target, value_t arg0, value_t arg1 );

value_t CallIOAction( zone_t zone, value_t action );
bool IsAnIOAction( value_t action );

// For the convenience of IOAction functions:
#define IOACTION_SLOT_COUNT 1

#endif	//ioaction_h

