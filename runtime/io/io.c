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


#include "io.h"
#include "libradian.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "strings.h"
#include "symbols.h"
#include "fixints.h"
#include "exceptions.h"
#include "closures.h"
#include "booleans.h"
#include "ioaction.h"
#include "basicio.h"
#include "loadexternal.h"
#include "callout.h"
#include "tuples.h"
#include "collector.h"
#include "debugtrace.h"

#define IO_SLOT_COUNT 2
#define IO_PROCEDURE_SLOT 0
#define IO_ARG_SLOT 1

static value_t IO_function( PREFUNC, value_t selector );

static value_t IO_print( PREFUNC, value_t io, value_t string )
{
	ARGCHECK_2( io, string );
	static struct closure proc = {(function_t)IOAction_Print_1};
	return MakeAsyncIOAction_1( zone, &proc, string );
}

static value_t IO_input( PREFUNC, value_t io )
{
	ARGCHECK_1( io );
	static struct closure proc = {(function_t)IOAction_Input_0};
	return MakeAsyncIOAction_0( zone, &proc );
}

static value_t IO_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(print, IO_print)
	DEFINE_METHOD(input, IO_input)
	return ThrowCStr(
			zone, "the IO object does not have the requested member" );
}

static struct closure ioapi = {(function_t)IO_function};

static void AbortIfException( value_t op )
{
	if (!IsAnException( op )) return;
	DumpToStderr( root_zone(), op );
	fprintf( stderr, "\n" );
	exit( 255 );
}

static void AbortIfNotIOAction( value_t op )
{
	if (IsAnIOAction( op )) return;
	fprintf( stderr, "program response not an IO action:\n" );
	DumpToStderr( root_zone(), op );
	fprintf( stderr, "\n" );
	exit( 255 );
}

// RunIO
//
// Main entry point for the radian program, after the runtime has been set up
// and the args have been collected. This is the loop that performs IO actions
// yielded back from the program's async task.
//
// The proc argument is the Radian main function. It expects to receive an IO
// object parameter and the argument parameter. Its result will be an async
// task object. We will start that task and run it to completion. Each response
// will be an IO action, except the last: the final response will be an integer
// result code.
//
int RunIO( value_t proc, value_t argument )
{
	AbortIfException( proc );

	// We'll need an allocation zone for long-lived data. This is not the same
	// as the root zone, which contains eternal data; it is an intermediate
	// zone containing the application's working data.

	// Call the main function to get the program's main async task, then start
	// the task.
	zone_t zone = zone_create();
	value_t task = CALL_2( proc, &ioapi, argument );
	AbortIfException( task );
	task = METHOD_0( task, sym_start );

	while (true) {
		AbortIfException( task );
		// As long as the task is running, its responses must be IO actions;
		// we will execute them and send the result back in.
		value_t is_running = METHOD_0( task, sym_is_running );
		AbortIfException( is_running );
		if (!BoolFromBoolean( zone, is_running )) {
			break;
		}
		value_t ioaction = METHOD_0( task, sym_response );
		AbortIfException( ioaction );
		AbortIfNotIOAction( ioaction );
		value_t result = CallIOAction( zone, ioaction );
		task = METHOD_1( task, sym_send, result );
		// Release any temporary data we allocated while producing that action.
		// This will move the task on to the next zone. We will start the next
		// loop iteration with a fresh scratch zone.
		zone_t next = zone_create();
		task = collect_zone( zone, task, next );
		zone = next;
	}

	// Once the async task is finished, the last response will be the result
	// value which we will return from the C main entrypoint.
	value_t response = METHOD_0( task, sym_response );
	AbortIfException( response );
	int errcode = IntFromFixint( response );
	return errcode;
}

