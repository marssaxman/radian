// Copyright 2010-2012 Mars Saxman
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

// Right, so this is the big piece of magic that makes the rest of the language
// worth something.
// The goal is to parallelize as much work as possible. We assume that the only
// work that matters is work done inside a loop. If it's not in a loop, then it
// either takes effectively no time, or it calls something that contains a
// loop. Any way about it, the loops are where the action is.
// Radian has two loops: a simple conditional while-loop, and a sequence-driven
// for-loop. The for-loop is the more interesting of the two. Sequences are
// lazy, so the act of iterating over a sequence is separated from the act of
// computing each element value of a sequence. Ideally, a sequence's "next"
// function should be very cheap, and the "current" function should do all the
// actual calculation.
// We can divide the for-loop into two parts: the work it takes to compute the
// values of the input sequence, which we will call the "map" stage, and the
// work that we do as an accumulation over the values of the sequence, which we
// will call the "reduce" stage. While each iteration of the reduce step
// depends on the value computed in the previous one, the map stage can be
// executed independently. Furthermore, multiple mappings can be executed
// simultaneously. Our goal is to maximize throughput. 

// case: reduce time == map time
//	strategy: use two cores, reduce on one core, map on the other
// case: reduce time < map time, reduce time * (N - 1) >= map time
//	strategy: use N cores, reduce on one core, map on all others
// case reduce time < map time, reduce time * (N - 1) < map time
//  strategy: use N cores, reduce on one core, map on all others, map on all
//	cores when blocked
// case: reduce time > map time
//	strategy: use two cores, reduce on one core, map on the other, sleep between

// If the reduce stage takes a long time, it is probably because it contains
// some inner loops. The other cores can keep busy by working on the map stages
// of those nested loops, but we still need to keep the reduce stage going full
// time.  Still, we would always like to work on the biggest chunks we can, so
// we will try to keep the inner loops running in serial mode as much as
// possible.

#include "parallel.h"
#include "closures.h"
#include "exceptions.h"
#include "symbols.h"
#include "booleans.h"
#include "platform/threads.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "macros.h"

// iterator object slots
//	0: wrapped iterator
//	1: calculated "is_valid" value
//	2: calculated "current" value
//	3: calculated "next" value (this is a wrapper)
#define ITERATOR_SLOT_COUNT 4
#define ITERATOR_WRAPPED_SLOT 0
#define ITERATOR_VALID_SLOT 1
#define ITERATOR_CURRENT_SLOT 2
#define ITERATOR_NEXT_SLOT 3
static value_t wrap_source_iterator( zone_t zone, value_t original );
static void *worker_main( void *arg );

// an iterator can be in three states:
// not started yet
// in progress
// all finished
// the iterator is kind of a future.

static unsigned int s_num_workers;
struct worker {
	volatile value_t task; // null if no current task
	thread_t thread;
	thread_mutex_t lock;    // protects the "task" variable
	zone_t zone;            // the results should end up here
};
typedef struct worker worker_t;
static worker_t *s_workers;

void init_parallel(void)
{
	// Find out how many cores this machine has.
	// Allocate N - 1 workers.
	unsigned int num_cores = thread_count_procs();
	assert( num_cores > 0 );
	s_num_workers = num_cores - 1;

	if (s_num_workers > 0) {
		s_workers = (worker_t*)calloc( s_num_workers, sizeof(worker_t) );
		// allocate a pthread for each worker
		for (unsigned int i = 0; i < s_num_workers; i++) {
			worker_t *it = &s_workers[i];
			thread_mutex_create( &it->lock );
			thread_create( &it->thread, worker_main, it );
		}
	}
}

static void queue_iterator( zone_t zone, value_t task )
{
	// Look for an idle worker and try to convince it to get busy prefetching
	// the values in this iterator. It's OK if we don't succeed; all workers
	// might already be committed. We don't actually care whether the iterator
	// gets queued: we just want to make sure all the workers are busy. This
	// should be as quick a process as we can manage since it is loop overhead.
	bool assigned = false;
	for (unsigned int i = 0; i < s_num_workers && !assigned; i++) {
		// If this worker appears to be idle, try to lock it. Once we've locked
		// it, check again, in case someone else assigned it a task just before
		// we acquired the lock. If the worker is still idle, give it our task.
		if (s_workers[i].task) continue;
		if (thread_mutex_trylock( &s_workers[i].lock )) continue;
		// we've locked the worker: now we have a chance to assign it our task.
		// If nobody else assigned it a task while we were taking out the lock,
		// we'll do so now.
		if (NULL == s_workers[i].task) {
			// The worker thread itself does not block on its own mutex when
			// checking the state of its task variable. Assigning a non-null
			// value to the task variable must be the last thing we do, since
			// that will unblock the worker, and it will expect to have a zone
			// to do allocation in.
			s_workers[i].zone = zone;
			s_workers[i].task = task;
			assigned = true;
		}
		assert( 0 == thread_mutex_unlock( &s_workers[i].lock ) );
	}
}

static bool is_work_finished( value_t task )
{
	// As seen in process_iterator(), we know that the iterator is done when
	// its wrapped iterator is gone. We need the original data only until we
	// have finished flattening the wrapper; then the wrapper contains all the
	// result data.
	return (NULL == task->slots[0]);
}

static bool is_work_in_progress( value_t wrapper )
{
	// Is one of our workers currently digging away at this iterator?
	for (unsigned int i = 0; i < s_num_workers; i++) {
		if (s_workers[i].task == wrapper) return true;
	}
	return false;
}

static void process_iterator( zone_t zone, value_t it )
{
	if (IsAnException( it )) return;
	// Flatten out this iterator, right here and now. We are a wrapper around
	// some source-sequence iterator; our job is to memoize all of its
	// functions. This is technically a violation of object immutability, but
	// it has to happen somewhere. We'll never let anyone look at the contents
	// of a wrapper iterator before flattening it, and there is no way to clone
	// it (or to end up with two wrappers for the same target), so it still
	// works like an immutable object even if we actually change its contents.
	struct closure *iter = CLOSURE(it);

	value_t source = iter->slots[ITERATOR_WRAPPED_SLOT];
	if (!source) return;

	// Find out whether the iterator is valid, or is the terminator.
	value_t valid = METHOD_0( source, sym_is_valid );
	iter->slots[ITERATOR_VALID_SLOT] = valid;

	if (BoolFromBoolean( zone, valid )) {
		// Get a reference to the next iterator in the sequence. We do this
		// before evaluating the element value in hopes of getting it running
		// on a(nother) worker.
		value_t next = METHOD_0( source, sym_next );
		iter->slots[ITERATOR_NEXT_SLOT] = wrap_source_iterator( zone, next );

		// Evaluate the element value. If we are lucky, this will take a long
		// time.
		value_t current = METHOD_0( source, sym_current );
		iter->slots[ITERATOR_CURRENT_SLOT] = current;
	}
	
	// Zero out the source reference. This is how we signal that the iterator
	// wrapper has already been flattened and does not need any more processing.
	iter->slots[ITERATOR_WRAPPED_SLOT] = NULL;
}

static void *worker_main(void *arg)
{
	// Welcome to the worker thread.
	// We live to serve. The argument points at our worker_t struct, which is
	// where we will find instructions. The worker does not ever actually die;
	// it just eventually gets killed when the app exits. Our job is to process
	// iterators. If there are no iterators waiting to be processed, we do
	// nothing. We clear out our task variable to signal that we are done with
	// it. Some future process, wishing to get its own iterator going, will
	// assign a new task, which we will duly execute, ad infinitum.
	worker_t *self = (worker_t*)arg;
	while (true) {
		if (self->task) {
			assert( !is_work_finished( self->task ) );
			assert( self->zone );
			// There is a problem here. We do the iterator-processing work in
			// the worker's zone, but the data needs to end up back in the
			// zone belonging to whomever is asking for the iterator's value.
			// Perhaps we should create a temporary zone for use during this
			// process_iterator call, then copy/collect the data back when the
			// client asks for it.
			process_iterator( self->zone, self->task );
			self->zone = NULL;
			self->task = NULL;
		}
	}
	return NULL;
}

static void master_do_work( zone_t zone, value_t iter )
{
	// Reduce loop wants data from this iterator. Do some work until it is
	// ready to go. We will work on this item if it is not already in progress;
	// otherwise we will work on whatever is at the end of the chain.
	while (!is_work_finished(iter)) {
		// Walk down the iterator chain looking for some useful work to do. If
		// we find an iterator that has been created but is not yet in progress,
		// we'll yank it for ourselves.
		value_t next = iter;
		while (next && (is_work_finished(next) || is_work_in_progress(next))) {
			next = next->slots[ITERATOR_NEXT_SLOT];
		}
		if (next) {
			process_iterator( zone, next );
		}
		else {
			// We are waiting on an item which is not ready, but there is no
			// work for us to do. This is bad! For now we will just spinwait,
			// but someday we should come up with an architecture that lets the
			// master do something useful while waiting for its workers. This
			// might be less of a problem when we have some concurrency system;
			// this master can just put itself to sleep and let some other task
			// take over for a while.
		}
	}
}

static value_t pariter_is_valid( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return iter->slots[ITERATOR_VALID_SLOT];
}

static value_t pariter_current( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return iter->slots[ITERATOR_CURRENT_SLOT];
}

static value_t pariter_next( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	return iter->slots[ITERATOR_NEXT_SLOT];
}

static value_t pariter( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	master_do_work( zone, self );
	DEFINE_METHOD(is_valid, pariter_is_valid)
	DEFINE_METHOD(current, pariter_current)
	DEFINE_METHOD(next, pariter_next)
	return ThrowCStr( zone, "iterator does not have the requested method" );
}

static value_t wrap_source_iterator( zone_t zone, value_t original )
{
	if (IsAnException( original )) return original;
	// Here's an iterator from the original sequence. Wrap it in one of our
	// parallelized iterators, then try to get one of our workers going on it.
	struct closure *out = ALLOC( pariter, ITERATOR_SLOT_COUNT );
	out->slots[ITERATOR_WRAPPED_SLOT] = original;
	queue_iterator( zone, out );
	return out;
}

static value_t parseq_iterate( PREFUNC, value_t sequence )
{
	ARGCHECK_1( sequence );
	// Retrieve our target sequence. Call its "iterate" method to retrieve its
	// first iterator. Wrap that iterator in our populated-iterator wrapper,
	// which will compute and pull the "current" value and then keep pulling
	// iterators until it has gotten all of the workers in on the game.
	value_t original_sequence = sequence->slots[0];
	value_t src_iterator = METHOD_0( original_sequence, sym_iterate );
	if (IsAnException( src_iterator )) return src_iterator;
	return wrap_source_iterator( zone, src_iterator );
}

static value_t parseq( PREFUNC, value_t selector )
{
	// dispatcher function for the parallel-sequence object
	ARGCHECK_1( selector );
	DEFINE_METHOD(iterate, parseq_iterate)
	return ThrowCStr( zone, "sequence does not have the requested method" );
}

static value_t parallelize_func( PREFUNC, value_t exp )
{
	ARGCHECK_1( exp );
	// If we are running on a single-core machine, there is no point in
	// attempting to parallelize anything.
	if (s_num_workers <= 1) return exp;
	// Exp is some sequence. We will wrap this sequence in the parallel
	// evaluation engine. It would make no sense to parallelize a parallel
	// sequence, and anyway that should be impossible.
	assert( exp->function != (function_t)parseq );
	struct closure *out = ALLOC( parseq, 1 );
	out->slots[0] = exp;
	return out;
}
struct closure parallelize = {(function_t)parallelize_func};

