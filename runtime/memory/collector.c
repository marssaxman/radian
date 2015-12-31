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

// This is a simple mark-copy collector. Its performance is constant in space;
// in time it is O(n) on the number of objects in the source zone. It is a
// "stop the world" architecture, but only for the two allocation zones
// involved. The idea is that we will create a hierarchy of short-lived
// allocation zones associated with specific tasks, then collapse each child
// zone back into its parent on task completion.

// The collector is intimately familiar with the details of the allocation zone
// data structures and with the layout of an object. This coupling is inelegant,
// but we intend to run the collector frequently, and keeping its overhead to a
// minimum is an important part of the Radian performance architecture. Think
// of the allocator, closure maker, and collector as one interrelated system of
// memory management, not as a group of independent modules.

#include "pagealloc.h"
#include "collector.h"
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "zone-internal.h"

// Copying is a destructive operation: it implicitly destroys the source zone.
// As we copy each object, we will store a forwarding reference to the new
// object in the destination zone; this allows us to avoid copying the same
// object more than once. After copying an object, we reuse its first two words
// as a forward reference. We expect that every object will be at least two
// words large: that is, the function_t pointer plus one additional data word.
// We can safely make this assumption because we will only ever copy objects
// which belong to one of our allocation zones, and the zone allocator asserts
// that the object's data size is greater than zero, and further rounds its
// allocations up to whole word lengths. If the zone allocator stopped doing
// either of these things, we would have a problem.

struct forwarder
{
	uintptr_t flag;
	value_t target;
};
typedef struct forwarder *forwarder_t;
#define COLLECTOR_REDIRECT_FLAG ((uintptr_t)-1)
#define IS_COLLECTOR_REDIRECT(obj) \
		(((forwarder_t)(obj))->flag == COLLECTOR_REDIRECT_FLAG)
#define GET_REDIRECT_TARGET(obj) (((forwarder_t)(obj))->target)

static value_t copy_object( zone_t src, value_t obj, zone_t dest );

static void make_forwarder( value_t obj, value_t target )
{
	forwarder_t fwd = (forwarder_t)obj;
	fwd->flag = COLLECTOR_REDIRECT_FLAG;
	fwd->target = target;
}

static value_t copy_buffer( value_t obj, zone_t dest )
{
	// We know that this object is a buffer and not a closure, so its data is
	// a blob and not an array of object references. We must make a bitwise
	// copy in the destination zone, then update the original object so that it
	// points at the new copy.
	struct buffer *buf = (struct buffer*)obj;
	buf = clone_buffer( dest, buf->function, buf->size, buf->bytes );
	return (value_t)buf;
}

static value_t copy_closure(
		value_t obj, size_t objsize, zone_t src, zone_t dest )
{
	// The object is a closure, so we need to recursively copy its slots. We
	// are of course handwaving past the cost of the stack space necessary to
	// perform this graph traversal, so this algorithm is only suitable for
	// small demonstration purposes. We will likely have to grab some scratch
	// pages from the VM to use as a subsidiary stack while we crawl the graph.
	// It would be nice if there were some heuristic we could use to determine
	// when this is necessary, then fall back to plain ol' recursion when we
	// can safely get away with it.
	size_t nslots = (objsize - sizeof(struct closure)) / sizeof(value_t);
	struct closure *out = alloc_object( dest, obj->function, nslots );
	for (unsigned i = 0; i < nslots; i++) {
		out->slots[i] = copy_object( src, obj->slots[i], dest ); 
	}
	return out;
}

static bool is_buffer( size_t objsize, value_t obj )
{
	// Determining whether an object is a buffer or a closure is trickier than
	// it seems like it ought to be. I've wasted enough hours on it already, so
	// I'll use this simple kludge for now, and come back to fix it when I have
	// whittled down the to-do list some more. We know that a buffer stores its
	// content size as the first field following the function pointer: so we'll
	// peek at that slot and see if it happens to match up with the size of the
	// object itself, as tracked by the zone manager. This will of course fail
	// in the extraordinarily unlikely situation that an extraordinarily large
	// closure happens to contain, as its first slot, a reference to an object
	// which just happens to live at an address almost equal to the closure's
	// size. Cue data corruption and an inexplicable crash.
	size_t candidate = ((struct buffer*)obj)->size + sizeof(struct buffer);
	size_t word_size = sizeof(void*);
	candidate += (word_size - 1);
	candidate &= ~(word_size - 1);
	return objsize == candidate;
}

static value_t copy_object( zone_t src, value_t obj, zone_t dest )
{
	// Null references are rare, since you can't express one directly from
	// radian code, but we must handle them in case the C runtime uses one.
	if (!obj) return obj;

	// Get the page header structure for this object. That will tell us how
	// large it is and to which zone it belongs. We can do this by masking off
	// the pointer bits which represent the location within a page.
	uintptr_t pagemask = PAGE_SIZE - 1;
	struct zone_page *zp = (struct zone_page*)(((uintptr_t)obj) & ~pagemask);

	// If the object does not belong to the source zone, there is no need to
	// copy it, because it will continue to exist after we destroy the zone.
	if (zp->owner != src) {
		return obj;
	}

	// The only field an object is guaranteed to have is its function pointer,
	// so we bastardize that field shamelessly during the collection process
	// to signal that an old object has already been forwarded to a new one.
	if (IS_COLLECTOR_REDIRECT(obj)) {
		return GET_REDIRECT_TARGET(obj);
	}
		
	// If this object is a buffer, we just blit its contents into a new buffer.
	// If it is a closure, we must recursively copy its slots. 
	size_t objsize = zp->element_size;
	value_t out = NULL;
	if (is_buffer( objsize, obj )) {
		out = copy_buffer( obj, dest );
	} else {
		out = copy_closure( obj, objsize, src, dest );
	}
	make_forwarder( obj, out );
	return out;
}

value_t collect_zone( zone_t src, value_t root, zone_t dest )
{
	assert( src );
	assert( dest );
	assert( src != dest );
	// Given a root object, copy all live objects from the source zone to the
	// destination zone, then free the source zone. Return the copied root
	// object, now that it lives in the dest zone.
	// This is obviously a destructive operation, so you can only use it when
	// the task using the source zone has returned; you must guarantee that
	// no-one but the collector is reading from or writing to src.
	root = copy_object( src, root, dest );
	zone_destroy( src );
	return root;
}
