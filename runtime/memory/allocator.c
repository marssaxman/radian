// Copyright 2011-2013 Mars Saxman
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

#include "allocator.h"
#include "pagealloc.h"
#include "threads.h"
#include <assert.h>
#include "zone-internal.h"



zone_t zone_create(void)
{
	// Grab a page from the VM. This will contain the master zone index.
	// The beginning of the master zone index will be our zone struct.
	// The remaining space in the master zone page is divided between an index
	// of small-block groups and an index of large blocks.
	zone_t out = (zone_t)page_alloc();

	// Only one thread can allocate or free memory in a zone at a time.
	thread_mutex_create( &out->lock );

	// The small block group index always occupies exactly half of a page, so
	// we will start it right at the middle of the master index page.
	// Curiously, the large block pointer also begins at the middle of the
	// page, because we allocate backwards until we fill up the available space.
	// This is the same order we allocate blocks inside group pages.
	void *midpoint = (char*)out + PAGE_SIZE / 2;
	out->small_blocks = (group_t*)midpoint;
	out->large_blocks = (large_block_t*)midpoint;

	return out;
}

static void free_group( group_t group )
{
	while (group) {
		group_t next = group->previous;
		page_free( group );
		group = next;
	}
}

static void free_large_block( large_block_t block )
{
	multipage_free( block, 
			sizeof(struct large_block) + block->header.element_size );
}

static void free_overflow( group_t overflow )
{
	// This group contains pointers to large blocks.
	while (overflow) {
		group_t next = overflow->previous;
		
		large_block_t *end = (large_block_t*)((char*)overflow + PAGE_SIZE);
		char *begin = 
				(char*)overflow + sizeof(struct group) + overflow->available;
		large_block_t *target = (large_block_t*)begin;
		while (target < end) {
			free_large_block( *target );
			target++;
		}
		
		overflow = next;
	}
}

void zone_destroy( zone_t zone )
{
	// We are done with this zone. Release all of its pages back to the VM.
	
	// Small blocks live in lists of same-size blocks.
	unsigned int group_count = PAGE_SIZE / sizeof(void*) / 2;
	for (unsigned int i = 0; i < group_count; i++) {
		free_group( zone->small_blocks[i] );
	}
	
	// Large blocks are allocated in contiguous page groups; we have some
	// in the master index and some, possibly, in overflow pages.
	large_block_t *end = (large_block_t*)zone->small_blocks;
	large_block_t *target = zone->large_blocks;
	while (target < end) {
		free_large_block( *target );
		target++;
	}
	free_overflow( zone->overflow );
	
	// Release the mutex.
	// Only one thread can allocate or free memory in a zone at a time.
	thread_mutex_destroy( &zone->lock );
	
	// Now that we've freed all the subsidiary pages, free up the zone's master
	// index page itself.
	page_free( zone );
}

static void *large_alloc( zone_t zone, size_t size )
{
	// First, ask the system VM for a set of pages large enough to hold the
	// block we want to create, plus the handful of bytes we will use for
	// maintenance overhead.
	size_t total_size = sizeof(struct large_block) + size;
	large_block_t block = (large_block_t)multipage_alloc( total_size );
	block->header.owner = zone;
	block->header.element_size = size;
	void *out = (char*)block + sizeof(struct large_block);

	// We need to keep track of this block somewhere so we can return it to the
	// system when we destroy the allocation zone. Most of the time we will 
	// store our large block pointers in the zone's master index page; if there
	// is still room there, that's what we will do.
	char *safepoint = (char*)zone + sizeof(struct zone) + sizeof(large_block_t);
	if ((char*)zone->large_blocks >= safepoint) {
		--zone->large_blocks;
		*zone->large_blocks = block;
	}
	else {
		// The master index of large blocks is full, so it's time to move into
		// overflow storage. This may be the first allocation that needed the
		// overflow space, or the existing overflow space may be full, so we
		// may need to allocate a new overflow group. 
		group_t group = zone->overflow;
		if (!group || group->available < sizeof(large_block_t)) {
			group_t old = group;
			group = (group_t)page_alloc();
			group->header.owner = zone;
			group->header.element_size = sizeof(large_block_t);
			group->available = PAGE_SIZE - sizeof(struct group);
			group->previous = old;
			zone->overflow = group;
		}
		// We definitely have a valid overflow group now. Decrement the bytes
		// available count and use it to compute the location of our new
		// storage pointer. The available bytes always come at the beginning
		// of the page - we work backwards from the end.
		group->available -= sizeof(large_block_t);
		void *dest = (char*)group + sizeof(struct group) + group->available;
		*(large_block_t*)dest = block;
	}

	// The pointer we return is not that of the block header, but of its data
	// payload, which is what our caller actually cares about.
	return out;
}

static void *small_alloc( zone_t zone, size_t size )
{
	// Divide the byte size by word size to get the index of the group page
	// this small block will live in, then retrieve the pointer to that page.
	unsigned int group_index = (size / sizeof(void*)) - 1;
	assert( group_index < PAGE_SIZE / 2 );
	group_t group = zone->small_blocks[group_index];
		
	// If this is the first group page we have allocated for this size, or the
	// existing page has filled up, then we must allocate a new group page.
	if (!group || group->available < size) {
		group_t old = group;
		group = (group_t)page_alloc();
		group->header.owner = zone;
		group->header.element_size = size;
		group->available = PAGE_SIZE - sizeof(struct group);
		group->previous = old;
		zone->small_blocks[group_index] = group;
	}
	
	// We definitely have a valid group page now. Decrement the available-bytes
	// pointer and compute the address of the block we just allocated. This way
	// we allocate objects from the end of the page back to the beginning,
	// which is an important part of our garbage collection strategy.
	group->available -= size;
	void *out = (char*)group + sizeof(struct group) + group->available;

	// Pages start out zero-filled, and the allocator does not reuse allocated
	// blocks, so the returned buffer will also be zero-filled.
	return out;
}

void *zone_alloc( zone_t zone, size_t size )
{
	// We must always allocate at least some data, or what's the point of the
	// object's existence? This should never happen. What's more, the collector
	// requires that it can never happen.
	assert( size > 0 );
	
	// Round the allocation size up to the nearest word.
	size_t word_size = sizeof(void*);
	size += (word_size - 1);
	size &= ~(word_size - 1);
	
	// Only one thread gets to allocate from a zone at a time.
	thread_mutex_lock( &zone->lock );
	
	// If the size is above the threshold, go allocate it as a large block;
	// otherwise, keep going here to allocate a small block.
	size_t threshold = (PAGE_SIZE - sizeof(struct group)) / 2;
	void *out = NULL;
	if (size >= threshold) {
		out = large_alloc( zone, size );
	} else {
		out = small_alloc( zone, size );
	}
	
	// We're done allocating, so we can release the lock and let some other
	// thread have a turn at this zone.
	thread_mutex_unlock( &zone->lock );
	
	return out;
}



