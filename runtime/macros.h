// Copyright 2012-2013 Mars Saxman
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

#ifndef macros_h
#define macros_h

#include "closures.h"
#include "exceptions.h"
#include <stdio.h>

// When built with RUN_TESTS=1, the library will run a series of unit tests at
// init time. Don't use this for normal builds!
#ifndef RUN_TESTS
#define RUN_TESTS 0
#endif


// Every value_t is an invokable, either a closure or a buffer, whose first
// member is a pointer to some code intented to do something useful with its
// associated data. These are some helpful functions for making invokables and
// for getting at their data.
#define CLOSURE(x) ((struct closure*)x)
#define ALLOC(function, slotcount) \
	alloc_object(zone, (function_t)function, slotcount)
#define BUFFER(x) ((struct buffer*)x)
#define BUFDATA(x,type) ((type*)(BUFFER(x)->bytes))
#define BUFALLOC(function, bytes) \
	alloc_buffer(zone, (function_t)function, bytes)


// These macros simplify the process of calling an invokable from C code.
#define CALL_1(target, arg0) \
	target->function(zone,(target),1,(arg0))
#define CALL_2(target, arg0, arg1) \
	target->function(zone,(target),2,(arg0),(arg1))
#define CALL_3(target, arg0, arg1, arg2) \
	target->function(zone,(target),3,(arg0),(arg1),(arg2))


// An object is an invokable which accepts one parameter: a symbol identifying
// the member function you are interested in. Each member function accepts one
// implicit first parameter: you must tell it which object it came from.
#define METHOD_0(target, sym) \
	method_0(zone, (target), (sym))
#define METHOD_1(target, sym, arg0) \
	method_1(zone, (target), (sym), (arg0))
#define METHOD_2(target, sym, arg0, arg1) \
	method_2(zone, (target), (sym), (arg0), (arg1))


// When defining an object dispatcher function, this may be convenient:
#define DEFINE_METHOD(name, function) \
	if (sym_##name == selector) { \
		static struct closure out = {(function_t)function}; \
		return &out; \
	}


// Every function which may be called through the radian function protocol must
// begin with an appropriate argument check. This gives the expected exception
// behavior when the argument count doesn't match or when one of the arguments
// is an illegal (exceptional) value.
#ifndef TRACE
#define TRACE 0
#endif
#define ARGCHECK_0 \
	if (TRACE) fprintf( stderr, "%s\n", __func__ ); \
	if (argc != 0) return ThrowArgCountFail( zone, __func__, 0, argc );
#define ARGCHECK_1(arg0) \
	if (TRACE) fprintf( stderr, "%s\n", __func__ ); \
	if (argc != 1) return ThrowArgCountFail( zone, __func__, 1, argc ); \
	if (IsAnException(arg0)) return (arg0);
#define ARGCHECK_2(arg0, arg1) \
	if (TRACE) fprintf( stderr, "%s\n", __func__ ); \
	if (argc != 2) return ThrowArgCountFail( zone, __func__, 2, argc ); \
	if (IsAnException(arg0)) return (arg0); \
	if (IsAnException(arg1)) return (arg1);
#define ARGCHECK_3(arg0, arg1, arg2) \
	if (TRACE) fprintf( stderr, "%s\n", __func__ ); \
	if (argc != 3) return ThrowArgCountFail( zone, __func__, 3, argc ); \
	if (IsAnException(arg0)) return (arg0); \
	if (IsAnException(arg1)) return (arg1); \
	if (IsAnException(arg2)) return (arg2);

#endif	//macros_h
