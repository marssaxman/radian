// Copyright 2011 Mars Saxman
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

#ifndef list_chunks_h
#define list_chunks_h

#include <stdbool.h>
#include "closures.h"

bool IsAChunk( value_t chunk );
bool chunk_can_grow( value_t chunk );
bool chunk_can_shrink( value_t chunk );
int chunk_leaf_count( value_t chunk );
int chunk_item_count( value_t chunk );
value_t chunk_alloc_1( zone_t zone, value_t value );
value_t chunk_push( zone_t zone, value_t chunk, value_t value );
value_t chunk_pop( zone_t zone, value_t chunk );
value_t chunk_head( value_t chunk );
value_t chunk_append( zone_t zone, value_t chunk, value_t value );
value_t chunk_chop( zone_t zone, value_t chunk );
value_t chunk_tail( zone_t zone, value_t chunk );
value_t chunk_get_leaf( zone_t zone, value_t chunk, int index );

#endif	//list_chunks_h
