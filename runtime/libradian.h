// Copyright 2009-2013 Mars Saxman
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

#ifndef libradian_h
#define libradian_h

#include <stdlib.h>
#include "memory/allocator.h"
#include "memory/collector.h"
#include "closures.h"
#include "buffer.h"
#include "atoms/numbers.h"
#include "atoms/booleans.h"
#include "atoms/relations.h"
#include "atoms/stringliterals.h"
#include "atoms/symbols.h"
#include "atoms/floats.h"
#include "io/io.h"
#include "io/ioargs.h"
#include "io/callout.h"
#include "io/loadexternal.h"
#include "io/basicio.h"
#include "containers/tuples.h"
#include "containers/maps.h"
#include "containers/lists.h"
#include "containers/list-empty.h"
#include "flowcontrol.h"
#include "exceptions.h"
#include "parallel.h"
#include "macros.h"
#include "debugtrace.h"
#include "mathlib.h"

zone_t init_runtime(void);
zone_t root_zone(void);

#endif //libradian_h
