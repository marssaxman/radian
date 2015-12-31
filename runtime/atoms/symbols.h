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


#ifndef symbols_h
#define symbols_h

#include <stdbool.h>
#include "../closures.h"

// These vars must only ever be assigned from init_symbols(), which must be
// called from init_runtime(). Runtime code should never call SymbolLiteral()
// on a string literal; instead, use these global vars. Think of these as const,
// even though they can't be defined that way.
#define SYMBOL(x) extern value_t sym_##x
#include "symbol-list.h"
#undef SYMBOL

void init_symbols( zone_t zone );
value_t SymbolLiteral( zone_t zone, const char *data );
bool IsASymbol( value_t obj );
const char *CStrFromSymbol( value_t it );

#endif //symbols_h
