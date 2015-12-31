// Copyright 2010-2011 Mars Saxman
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

#include "threads.h"
#include <pthread.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <assert.h>
#include <stdlib.h>

struct thread_opaque { pthread_t it; };
struct thread_mutex_opaque { pthread_mutex_t it; };

void thread_create( thread_t *thread, thread_entry_t entry, void *arg )
{
	*thread = (thread_t)malloc(sizeof(struct thread_opaque));
	assert( 0 == pthread_create( &((*thread)->it), NULL, entry, arg ) );
}

void thread_mutex_create( thread_mutex_t *mutex )
{
	*mutex = (thread_mutex_t)malloc(sizeof(struct thread_mutex_opaque));
	assert( 0 == pthread_mutex_init( &((*mutex)->it), NULL ) );
}

void thread_mutex_destroy( thread_mutex_t *mutex )
{
	assert( 0 == pthread_mutex_destroy( &((*mutex)->it) ) );
	free( *mutex );
}

int thread_mutex_trylock( thread_mutex_t *mutex )
{
	return pthread_mutex_trylock( &((*mutex)->it) );
}

int thread_mutex_lock( thread_mutex_t *mutex )
{
	return pthread_mutex_lock( &((*mutex)->it) );
}

int thread_mutex_unlock( thread_mutex_t *mutex )
{
	return pthread_mutex_unlock( &((*mutex)->it) );
}

unsigned int thread_count_procs(void)
{
	uint32_t numCPU = 1;

	// use sysctl to ask for the number of available CPUs
	int mib[4];
	size_t len; 
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU; 
	sysctl( mib, 2, &numCPU, &len, NULL, 0 );

	// if we failed, try again with HW_NCPU instead
	if (numCPU < 1) {
		mib[1] = HW_NCPU;
		sysctl( mib, 2, &numCPU, &len, NULL, 0 );
	}
	
	// whatever happened, don't return a nonsensical value
	if (numCPU < 1) {
		numCPU = 1;
	}
	return numCPU;
}
