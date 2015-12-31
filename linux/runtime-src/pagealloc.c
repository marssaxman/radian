// Copyright 2011-2012 Mars Saxman
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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

void *page_alloc(void)
{
	void *out = valloc( PAGE_SIZE );
	memset( out, '\0', PAGE_SIZE );
	return out;
}

void page_free(void* page)
{
	free( page );
}

void *multipage_alloc( size_t bytes )
{
	void *out = valloc( bytes );
	memset( out, '\0', bytes );
	return out;
}

void multipage_free( void* block, size_t bytes )
{
	free( block );
}
