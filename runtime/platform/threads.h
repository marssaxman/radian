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

// Each platform's runtime must provide an implementation of these functions.

#ifndef threads_h
#define threads_h

typedef struct thread_opaque *thread_t;
typedef void *(*thread_entry_t)(void *);
void thread_create( thread_t*, thread_entry_t, void* );

typedef struct thread_mutex_opaque *thread_mutex_t;
void thread_mutex_create( thread_mutex_t* );
int thread_mutex_trylock( thread_mutex_t* );
int thread_mutex_lock( thread_mutex_t* );
int thread_mutex_unlock( thread_mutex_t* );
void thread_mutex_destroy( thread_mutex_t* );

unsigned int thread_count_procs(void);

#endif //threads_h
