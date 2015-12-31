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

// Lowest level allocator. Each platform must provide implementations of these
// functions. This is a thin interface to the VM, dealing in fixed size blocks.

#ifndef pagealloc_h
#define pagealloc_h

#include <stddef.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

// Allocate one page and return its base address. Page must be zero-filled.
void *page_alloc(void);

// Release one page back to the VM.
void page_free(void* page);

// Allocate enough pages to hold a block of the specified size.
void *multipage_alloc(size_t bytes);

// Release a multiple-page block. Yes, we have to keep track of its length.
void multipage_free(void* block, size_t bytes);

#endif	//pagealloc_h
