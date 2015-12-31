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

#include "pagealloc.h"
#include <mach/mach.h>


void *page_alloc(void)
{
	void *out = NULL;
	vm_allocate( mach_task_self(), (vm_address_t*)&out, PAGE_SIZE, 
		VM_FLAGS_ANYWHERE );
	return out;
}

void page_free(void* page)
{
	vm_deallocate( mach_task_self(), (vm_address_t)page, PAGE_SIZE );
}

void *multipage_alloc( size_t bytes )
{
	void *out = NULL;
	vm_allocate( mach_task_self(), (vm_address_t*)&out, bytes,
		VM_FLAGS_ANYWHERE );
	return out;
}

void multipage_free( void* block, size_t bytes )
{
	vm_deallocate( mach_task_self(), (vm_address_t)block, bytes );
}
