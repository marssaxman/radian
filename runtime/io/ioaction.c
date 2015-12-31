// Copyright 2012 Mars Saxman
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

#include <assert.h>
#include "ioaction.h"
#include "../exceptions.h"
#include "../atoms/symbols.h"
#include "../atoms/booleans.h"
#include "macros.h"

// An IOAction is a description of a stateful, world-mutating action that the
// main loop can perform. It is a token, something to return back from an async
// io task; it does nothing on its own. It is an object, since we have to be
// able to pass it around as a value, but it is not a normal invokable.
//
// An IOAction must contain the parameter values for the mutation, so we will
// allocate an arbitrary number of slots to contain their values. When we call
// the actual target, we will pass the IOAction itself in as an argument; the
// target is expected to break the normal protocol and peek into the IOAction's
// slots, retrieving its parameter values.
//
// This is all fairly tightly coupled, but we can get away with it because
// there are not many different possible IOAction targets: they are all defined
// inside runtime/io, and we intend to keep the list as short as possible. The
// great majority of IOActions will be FFI calls out to external libraries.
//
// The return from MakeAsyncIOAction is not, itself, an IOAction: it is a stub
// async task with a single step which responds with the IOAction. It works
// this way because the radian code expects all IO actions to be represented as
// async tasks, which it can then 'sync' from. Since the only thing the runtime
// ever does with an IOAction is return it to the program, we just create the
// async task up front when we create the IOAction.

#define IOACTION_TARGET_SLOT 0

#define ASYNCTASK_SLOT_COUNT 1
#define ASYNCTASK_TARGET_SLOT 0

#define ASYNCACTION_SLOT_COUNT 1
#define ASYNCACTION_TARGET_SLOT 0

static value_t AsyncAction_response( PREFUNC, value_t action )
{
	return action->slots[ASYNCACTION_TARGET_SLOT];
}

static value_t AsyncAction_send( PREFUNC, value_t action, value_t val )
{
	return ThrowCStr( zone, "task is not running" );
}

static value_t AsyncAction_function( PREFUNC, value_t selector )
{
	if (IsAnException( selector )) return selector;
	if (selector == sym_is_running) return &False_returner;
	DEFINE_METHOD( response, AsyncAction_response );
	DEFINE_METHOD( send, AsyncAction_send );
	return ThrowMemberNotFound( zone, selector );
}

static value_t WrapInAsyncAction( zone_t zone, value_t target )
{
	struct closure *out = ALLOC( AsyncAction_function, ASYNCACTION_SLOT_COUNT );
	out->slots[ASYNCACTION_TARGET_SLOT] = target;
	return out;
}



static value_t AsyncTask_start( PREFUNC, value_t task )
{
	return WrapInAsyncAction( zone, task->slots[ASYNCTASK_TARGET_SLOT] );
}

static value_t AsyncTask_function( PREFUNC, value_t selector )
{
	if (IsAnException( selector )) return selector;
	DEFINE_METHOD( start, AsyncTask_start );
	return ThrowMemberNotFound( zone, selector ); 
}

static value_t WrapInAsyncTask( zone_t zone, value_t target )
{
	struct closure *out = ALLOC( AsyncTask_function, ASYNCTASK_SLOT_COUNT );
	out->slots[ASYNCTASK_TARGET_SLOT] = target;
	return out;
}



static value_t IOAction_function( PREFUNC )
{
	return ThrowCStr( zone, "action tokens are not invokable" );
}

value_t MakeAsyncIOAction_0( zone_t zone, value_t target )
{
	struct closure *out = ALLOC( IOAction_function, IOACTION_SLOT_COUNT );
	out->slots[IOACTION_TARGET_SLOT] = target;
	return WrapInAsyncTask( zone, out );
}

value_t MakeAsyncIOAction_1( zone_t zone, value_t target, value_t arg0 )
{
	struct closure *out = ALLOC( IOAction_function, IOACTION_SLOT_COUNT + 1 );
	out->slots[IOACTION_TARGET_SLOT] = target;
	out->slots[IOACTION_SLOT_COUNT + 0] = arg0;
	return WrapInAsyncTask( zone, out );
}

value_t MakeAsyncIOAction_2(
		zone_t zone, value_t target, value_t arg0, value_t arg1 )
{
	struct closure *out = ALLOC( IOAction_function, IOACTION_SLOT_COUNT + 2 );
	out->slots[IOACTION_TARGET_SLOT] = target;
	out->slots[IOACTION_SLOT_COUNT + 0] = arg0;
	out->slots[IOACTION_SLOT_COUNT + 1] = arg1;
	return WrapInAsyncTask( zone, out );
}

value_t CallIOAction( zone_t zone, value_t action )
{
	assert( IsAnIOAction( action ) );
	return CALL_1( action->slots[IOACTION_TARGET_SLOT], action );
}

bool IsAnIOAction( value_t action )
{
	return action && action->function == (function_t)IOAction_function;
}

