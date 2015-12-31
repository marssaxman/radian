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


// PRIVATE MEMORY MANAGEMENT DATA STRUCTURES.
// Do not include this header unless you are part of the memory manager.


#ifndef zone_internal_h
#define zone_internal_h

#include "threads.h"

// We expect allocation block size to have a power-law relationship with the
// frequency such blocks are allocated: that is, the program will create a
// large number of small blocks, and a small number of large blocks.
// Our interface to the virtual memory manager is the page allocator: the VM
// will hand us memory in fixed units, whose size depends on the target OS
// and processor word size. On no platform we care about will the page size be
// smaller than 4096 bytes. We can ask for any number of contiguous pages.

// We will therefore divide allocations into "large" and "small" blocks, and
// use different strategies for each. The threshold is the size at which we
// can no longer fit more than one block into a page: that is, a small block is
// one which can share a page with another same-sized block.

typedef struct group *group_t;
typedef struct large_block *large_block_t;

struct zone
{
	thread_mutex_t lock;
	large_block_t *large_blocks;
	group_t overflow;
	group_t *small_blocks;
};

struct zone_page
{
	zone_t owner;
	size_t element_size;
};

struct group
{
	struct zone_page header;
	size_t available;
	group_t previous;
};

struct large_block
{
	struct zone_page header;
};

#endif	//zone_internal_h
