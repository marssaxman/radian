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

#include "callout.h"
#include "ffi-wrapper.h"
#include <stdlib.h>
#include <assert.h>
#include "loadexternal.h"
#include "exceptions.h"
#include "symbols.h"
#include "numbers.h"
#include "buffer.h"
#include "macros.h"

#define FUNC_SLOT_COUNT 4
#define FUNC_PTR_SLOT 0
#define FUNC_CIF_SLOT 1
#define FUNC_ARGS_SLOT 2
#define FUNC_RETVAL_SLOT 3

static value_t Func_function( PREFUNC )
{
	// An external function looks like it could be an invokable, but that would
	// wreck our nice pretty little immutable world. We don't know what the
	// external code will do, and we can't know, so we have to be safe and
	// invoke it as part of the IO action sequence. The only way to make this
	// code run is to use io.call.
	return ThrowCStr(
			zone, "external functions can only be invoked via io.call" );
}

static ffi_type * identify_ffi_type( zone_t zone, value_t marshal )
{
	// Given a marshalling object, figure out which FFI type it corresponds to.
	// This is easy, because we can just ask it.
	value_t ctype = METHOD_0( marshal, sym_ctype );
	if (ctype == sym_void) return &ffi_type_void;
	if (ctype == sym_int8) return &ffi_type_sint8;
	if (ctype == sym_uint8) return &ffi_type_uint8;
	if (ctype == sym_int16) return &ffi_type_sint16;
	if (ctype == sym_uint16) return &ffi_type_uint16;
	if (ctype == sym_int32) return &ffi_type_sint32;
	if (ctype == sym_uint32) return &ffi_type_uint32;
	if (ctype == sym_int64) return &ffi_type_sint64;
	if (ctype == sym_uint64) return &ffi_type_uint64;
	if (ctype == sym_pointer) return &ffi_type_pointer;
	return &ffi_type_void;
}

static value_t prep_cif( zone_t zone, value_t argtypes, value_t returntype )
{
	// Create a libffi CIF using our arg types and return type; it will live in
	// a buffer object, which is what we will return. The buffer will also need
	// to contain all the ffi_type instances we'll need to describe the arg and
	// return types.
	int argcount = IntFromFixint( METHOD_0( argtypes, sym_size ) );
	size_t bufsiz = sizeof(ffi_cif) + sizeof(ffi_type*) * argcount;
	struct buffer *out = Buffer( zone, bufsiz );
	ffi_cif *cif = (ffi_cif*)(&BUFFER(out)->bytes[0]);
	ffi_type **args = (ffi_type**)(&BUFFER(out)->bytes[sizeof(ffi_cif)]);
	ffi_type *ret = identify_ffi_type( zone, returntype );
	for (int i = 0; i < argcount; i++) {
		value_t marshal =
				METHOD_1( argtypes, sym_lookup, NumberFromInt( zone, i ) );
		args[i] = identify_ffi_type( zone ,marshal );
	}
	ffi_status stat = ffi_prep_cif( cif, FFI_DEFAULT_ABI, argcount, ret, args );
	return (stat == FFI_OK) ? (value_t)out :
		ThrowCStr( zone, "ffi_prep_cif failed" );
}

static value_t Describe_Function( PREFUNC,
	value_t pointer,
	value_t argtypes,
	value_t returntype )
{
	// Annotate this function with an array of argument types and a return
	// value type. By "type" we mean a marshalling object. We will also create
	// a libffi CIF, which the call command can use.
	if (IsAnException( pointer )) return pointer;
	if (!IsAPointer( pointer )) {
		return ThrowCStr(
				zone,
				"only a pointer can be described as an external function" );
	}
	if (IsAnException( argtypes )) return argtypes;
	if (IsAnException( returntype )) return returntype;
	value_t cif = prep_cif( zone, argtypes, returntype );
	if (IsAnException( cif )) return cif;
	struct closure *out = ALLOC( Func_function, FUNC_SLOT_COUNT );
	out->slots[FUNC_PTR_SLOT] = pointer;
	out->slots[FUNC_CIF_SLOT] = cif;
	out->slots[FUNC_ARGS_SLOT] = argtypes;
	out->slots[FUNC_RETVAL_SLOT] = returntype;
	return out;
}
const struct closure FFI_Describe_Function =
		{(function_t)Describe_Function};

static void* marshal_out( zone_t zone, value_t argval, value_t marshal )
{
	// Render some Radian value out as a buffer full of bytes which we can pass
	// off to some external function.
	value_t bytes = METHOD_1( marshal, sym_to_bytes, argval );
	int bufsiz = IntFromFixint( METHOD_0( bytes, sym_size ) );
	char *buf = malloc( bufsiz );
	for (int i = 0; i < bufsiz; i++) {
		value_t num = METHOD_1( bytes, sym_lookup, NumberFromInt( zone, i ) );
		buf[i] = IntFromFixint( num );
	}
	return buf;
}

static value_t IOAction_Call_2( PREFUNC, value_t action )
{
	assert( IsAnIOAction( action ) );
	value_t func = action->slots[ IOACTION_SLOT_COUNT + 0 ];
	value_t args = action->slots[ IOACTION_SLOT_COUNT + 1 ];

	// The only things you can call with io.call are functions you have
	// previously described using io.describe_function.
	if (func->function != (function_t)Func_function) {
		return ThrowCStr( zone, "only call external functions" );
	}

	value_t pointerObj = func->slots[FUNC_PTR_SLOT];
	value_t cifObj = func->slots[FUNC_CIF_SLOT];
	value_t argtypes = func->slots[FUNC_ARGS_SLOT];
	value_t returntype = func->slots[FUNC_RETVAL_SLOT];

	// C has an embarrassing hole: there is no legal way to cast a void pointer
	// to a function pointer. This is a legacy of the ancient past, when the
	// size or format of a pointer to data might have been different from a
	// pointer to code. This is not true of any architecture we care about, so
	// we will just work around the error message and get what we want anyway.
	void (*pointer)(void) = 0;
	*(void**)(&pointer) = *(void**)(BUFFER(pointerObj)->bytes);

	// Extract a pointer to the CIF from the object that contains it.
	ffi_cif *cif = (ffi_cif*)(BUFFER(cifObj)->bytes);

	// Marshal the argument values out to an array of byte buffers. We will
	// need one buffer per argument, and we have one marshaller per argument.
	int argcount = IntFromFixint( METHOD_0( args, sym_size ) );
	void **argvals = calloc( argcount, sizeof(void*) );
	for (int i = 0; i < argcount; i++) {
		value_t argval =
				METHOD_1( args, sym_lookup, NumberFromInt( zone, i ) );
		value_t marshal =
				METHOD_1( argtypes, sym_lookup, NumberFromInt( zone, i ) );
		argvals[i] = marshal_out( zone, argval, marshal );
	}

	// The return value buffer must be at least as large as sizeof(ffi_arg),
	// but it may be larger; if larger, we will allocate a buffer of the size
	// specified by the marshalling object.
	int retValSize = IntFromFixint( METHOD_0( returntype, sym_byte_size ) );
	if (retValSize < (int)sizeof(ffi_arg)) {
		retValSize = (int)sizeof(ffi_arg);
	}
	value_t rvalue_obj = (value_t)Buffer( zone ,retValSize );
	void *rvalue = BUFFER(rvalue_obj)->bytes;

	// Perform the call.
	ffi_call( cif, pointer, rvalue, argvals );

	// Marshal the return value back to a value from its byte buffer.
	value_t result = METHOD_1( returntype, sym_from_bytes, rvalue_obj );

	// Free up all those buffers we allocated for argument values - since we
	// didn't allocate them through the radian object allocator, we can't expect
	// the garbage collector to take care of them.
	for (int i = 0; i < argcount; i++) {
		free( argvals[i] );
	}
	free( argvals );

	return result;
}

static value_t FFI_Call_function( PREFUNC, value_t func, value_t args )
{
	ARGCHECK_2( func, args );
    static struct closure proc = {(function_t)IOAction_Call_2};
    return MakeAsyncIOAction_2( zone, &proc, func, args );
}
const struct closure FFI_Call = {(function_t)FFI_Call_function};
