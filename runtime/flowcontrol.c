// Copyright 2009-2012 Mars Saxman
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


#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "booleans.h"
#include "exceptions.h"
#include "flowcontrol.h"
#include "symbols.h"
#include "macros.h"

// slots for synchronous (normal)  loop object
#define LOOP_SLOT_COUNT 2
#define LOOP_CONDITION_SLOT 0
#define LOOP_OPERATION_SLOT 1

// slots for asynchronous sequence-generating loop object
#define LOOPSEQ_SLOT_COUNT 3
#define LOOPSEQ_CONDITION_SLOT 0
#define LOOPSEQ_OPERATION_SLOT 1
#define LOOPSEQ_VALUE_SLOT 2

// slots for asynchronous sequence-generating loop terminator object
#define LOOPSEQTERM_SLOT_COUNT 1
#define LOOPSEQTERM_VALUE_SLOT 0

// slots for asynchronous sequence-generating loop iterator object
#define LOOPITER_SLOT_COUNT 3
#define LOOPITER_CONDITION_SLOT 0
#define LOOPITER_OPERATION_SLOT 1
#define LOOPITER_ITER_SLOT 2

// slots for asynchronous task-generating loop object
#define LOOPTASK_SLOT_COUNT 3
#define LOOPTASK_CONDITION_SLOT 0
#define LOOPTASK_OPERATION_SLOT 1
#define LOOPTASK_VALUE_SLOT 2

// slots for asynchronous task-generating loop terminator object
#define LOOPTASKTERM_SLOT_COUNT 1
#define LOOPTASKTERM_VALUE_SLOT 0

// slots for asynchronous task-generating loop iterator object
#define LOOPACTION_SLOT_COUNT 3
#define LOOPACTION_CONDITION_SLOT 0
#define LOOPACTION_OPERATION_SLOT 1
#define LOOPACTION_ACTION_SLOT 2


static value_t Loop_Sequence_Decide(
		zone_t zone, value_t condition, value_t operation, value_t value );
static value_t Loop_Iterator_function( PREFUNC, value_t selector );

static value_t Loop_Task_Decide(
		zone_t zone, value_t condition, value_t operation, value_t value );
static value_t Loop_Action_function( PREFUNC, value_t selector );

static value_t Loop_Sequence_Terminator_Current( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	return iter_obj->slots[LOOPSEQTERM_VALUE_SLOT];
}

static value_t Loop_Sequence_Terminator_Next( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	return ThrowCStr( zone, "iterator is not valid" );
}

static value_t Loop_Sequence_Terminator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (selector == sym_is_valid) return &False_returner;
	DEFINE_METHOD( current, Loop_Sequence_Terminator_Current );
	DEFINE_METHOD( next, Loop_Sequence_Terminator_Next );
	return ThrowCStr( zone, "iterator does not implement that method" );
}

static value_t Loop_Iterator_current( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	value_t iter = iter_obj->slots[LOOPITER_ITER_SLOT];
	return METHOD_0( iter, sym_current );
}

static value_t Loop_Iterator_next( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	value_t iter = iter_obj->slots[LOOPITER_ITER_SLOT];
	value_t next = METHOD_0( iter, sym_next );
	if (IsAnException( next )) return next;
	value_t flag = METHOD_0( next, sym_is_valid );
	if (BoolFromBoolean( zone, flag )) {
		struct closure *out =
				ALLOC(Loop_Iterator_function, LOOPITER_SLOT_COUNT );
		out->slots[LOOPITER_CONDITION_SLOT] =
				iter_obj->slots[LOOPITER_CONDITION_SLOT];
		out->slots[LOOPITER_OPERATION_SLOT] =
				iter_obj->slots[LOOPITER_OPERATION_SLOT];
		out->slots[LOOPITER_ITER_SLOT] = next;
		return out;
	} else {
		value_t condition = iter_obj->slots[LOOPITER_CONDITION_SLOT];
		value_t operation = iter_obj->slots[LOOPITER_OPERATION_SLOT];
		value_t value = METHOD_0( next, sym_current );
		return Loop_Sequence_Decide( zone, condition, operation, value );
	}
}

static value_t Loop_Iterator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD( current, Loop_Iterator_current );
	DEFINE_METHOD( next, Loop_Iterator_next );
	if (selector == sym_is_valid) return &True_returner;
	return ThrowCStr( zone, "iterator does not implement that method" );
}

static value_t Loop_Sequence_Decide(
		zone_t zone, value_t condition, value_t operation, value_t value )
{
beginloop:
	if (IsAnException( value )) return value;
	value_t flag = CALL_1( condition, value );
	if (IsAnException( flag )) return flag;
	if (BoolFromBoolean( zone, flag )) {
		value_t seq = CALL_1( operation, value );
		if (IsAnException( seq )) return seq;
		// Run an iteration of the loop.
		value_t iter = METHOD_0( seq, sym_iterate );
		if (IsAnException( iter )) return iter;
		// If the iterator is done, return and try again.
		value_t valid = METHOD_0( iter, sym_is_valid );
		if (!BoolFromBoolean( zone, valid )) {
			value = METHOD_0( iter, sym_current );
			goto beginloop;
		}
		struct closure *out = ALLOC(
				Loop_Iterator_function, LOOPITER_SLOT_COUNT );
		out->slots[LOOPITER_CONDITION_SLOT] = condition;
		out->slots[LOOPITER_OPERATION_SLOT] = operation;
		out->slots[LOOPITER_ITER_SLOT] = iter;
		return out;
	} else {
		// Return a terminator which wraps the value. The caller will use
		// this value to retrieve the final values of all context variables
		// assigned during the loop body.
		struct closure *out = ALLOC(
				Loop_Sequence_Terminator_function, LOOPSEQTERM_SLOT_COUNT );
		out->slots[LOOPSEQTERM_VALUE_SLOT] = value;
		return out;
	}
}

static value_t Loop_Sequence_Iterate( PREFUNC, value_t loop_obj )
{
	ARGCHECK_1( loop_obj );
	// Pass the value through the condition. If it returns false, return a
	// terminal iterator. If it returns true, invoke the operation function
	// to retrieve a sequence. Open it, then wrap it in an async loop
	// iterator. In any case, the result is an iterator.
	value_t condition = loop_obj->slots[LOOPSEQ_CONDITION_SLOT];
	value_t operation = loop_obj->slots[LOOPSEQ_OPERATION_SLOT];
	value_t value = loop_obj->slots[LOOPSEQ_VALUE_SLOT];
	return Loop_Sequence_Decide( zone, condition, operation, value );
}

static value_t Loop_Sequence_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(iterate, Loop_Sequence_Iterate)
	return ThrowCStr( zone, "async loop does not implement that method" );
}

static value_t Loop_Sequencer(
		PREFUNC, value_t condition, value_t operation, value_t value )
{
	ARGCHECK_3( condition, operation, value );
	// Make a sequence which implements an asynchronous loop. We will yield
	// from the result of the operation function as long as the condition
	// function returns true.
	struct closure *out = ALLOC( Loop_Sequence_function, LOOPSEQ_SLOT_COUNT );
	out->slots[LOOPSEQ_CONDITION_SLOT] = condition;
	out->slots[LOOPSEQ_OPERATION_SLOT] = operation;
	out->slots[LOOPSEQ_VALUE_SLOT] = value;
	return out;
}

// ----------------

static value_t Loop_Task_Terminator_Response( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	return iter_obj->slots[LOOPSEQTERM_VALUE_SLOT];
}

static value_t Loop_Task_Terminator_Send( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	return ThrowCStr( zone, "task is not running" );
}

static value_t Loop_Task_Terminator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	if (selector == sym_is_running) return &False_returner;
	DEFINE_METHOD( response, Loop_Task_Terminator_Response );
	DEFINE_METHOD( next, Loop_Task_Terminator_Send );
	return ThrowCStr( zone, "task does not implement that method" );
}

static value_t Loop_Action_Response( PREFUNC, value_t iter_obj )
{
	ARGCHECK_1( iter_obj );
	value_t iter = iter_obj->slots[LOOPACTION_ACTION_SLOT];
	return METHOD_0( iter, sym_response );
}

static value_t Loop_Action_Send( PREFUNC, value_t action_obj, value_t value )
{
	ARGCHECK_2( action_obj, value );
	value_t action = action_obj->slots[LOOPACTION_ACTION_SLOT];
	value_t condition = action_obj->slots[LOOPACTION_CONDITION_SLOT];
	value_t operation = action_obj->slots[LOOPACTION_OPERATION_SLOT];

	value_t flag = METHOD_0( action, sym_is_running );
	if (IsAnException( flag )) return flag;
	assert( BoolFromBoolean( zone, flag ) );

	value_t next = METHOD_1( action, sym_send, value );
	if (IsAnException( next )) return next;

	flag = METHOD_0( next, sym_is_running );
	if (!BoolFromBoolean( zone, flag )) {
		value = METHOD_0( next, sym_response );
		return Loop_Task_Decide( zone, condition, operation, value );
	}

	struct closure *out =
			ALLOC(Loop_Action_function, LOOPACTION_SLOT_COUNT );
	out->slots[LOOPACTION_CONDITION_SLOT] = condition;
	out->slots[LOOPACTION_OPERATION_SLOT] = operation;
	out->slots[LOOPACTION_ACTION_SLOT] = next;
	return (value_t)out;
}

static value_t Loop_Action_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD( response, Loop_Action_Response );
	DEFINE_METHOD( send, Loop_Action_Send );
	if (selector == sym_is_running) return &True_returner;
	return ThrowCStr( zone, "action does not implement that method" );
}

static value_t Loop_Task_Decide(
		zone_t zone, value_t condition, value_t operation, value_t value )
{
	if (TRACE) fprintf( stderr, "Loop_Task_Decide\n" );
	// Pass the value through the condition.
	// If the result is true, run an iteration of the loop:
	//	Pass the value through the operation.
	//	Start the resulting task.
	//	Return a wrapper around the resulting step/action.
	// If the result is false, return a terminator.
	// This is different from the sequence generator: there is no such thing as
	// an empty task. A task always has at least one response, even if that is
	// the end of the task.
beginloop:
	if (IsAnException( value )) return value;
	value_t flag = CALL_1( condition, value );
	if (IsAnException( flag )) return flag;
	if (BoolFromBoolean( zone, flag )) {
		value_t seq = CALL_1( operation, value );
		if (IsAnException( seq )) return seq;
		// Run an iteration of the loop.
		value_t action = METHOD_0( seq, sym_start );
		if (IsAnException( action )) return action;
		struct closure *out = ALLOC(
				Loop_Action_function, LOOPACTION_SLOT_COUNT );
		out->slots[LOOPACTION_CONDITION_SLOT] = condition;
		out->slots[LOOPACTION_OPERATION_SLOT] = operation;
		out->slots[LOOPACTION_ACTION_SLOT] = action;
		value_t action_obj = (value_t)out;
		flag = METHOD_0( action_obj, sym_is_running );
		if (!BoolFromBoolean( zone, flag )) {
			value = METHOD_0( action_obj, sym_response );
			goto beginloop;
		}
		return out;
	} else {
		// Return a terminator which wraps the value. The caller will use
		// this value to retrieve the final values of all context variables
		// assigned during the loop body.
		struct closure *out = ALLOC(
				Loop_Task_Terminator_function, LOOPTASKTERM_SLOT_COUNT );
		out->slots[LOOPTASKTERM_VALUE_SLOT] = value;
		return out;
	}
}

static value_t Loop_Task_Start( PREFUNC, value_t loop_obj )
{
	ARGCHECK_1( loop_obj );
	// Pass the value through the condition. If it returns false, return a
	// terminal iterator. If it returns true, invoke the operation function
	// to retrieve a sequence. Open it, then wrap it in an async loop
	// iterator. In any case, the result is an iterator.
	value_t condition = loop_obj->slots[LOOPTASK_CONDITION_SLOT];
	value_t operation = loop_obj->slots[LOOPTASK_OPERATION_SLOT];
	value_t value = loop_obj->slots[LOOPTASK_VALUE_SLOT];
	return Loop_Task_Decide( zone, condition, operation, value );
}

static value_t Loop_Task_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(start, Loop_Task_Start)
	return ThrowMemberNotFound( zone, selector );
}

static value_t Loop_Task(
		PREFUNC, value_t condition, value_t operation, value_t value )
{
	ARGCHECK_3( condition, operation, value );
	struct closure *out = ALLOC( Loop_Task_function, LOOPTASK_SLOT_COUNT );
	out->slots[LOOPTASK_CONDITION_SLOT] = condition;
	out->slots[LOOPTASK_OPERATION_SLOT] = operation;
	out->slots[LOOPTASK_VALUE_SLOT] = value;
	return out;
}

// ----------------

const struct closure loop_sequencer = {(function_t)Loop_Sequencer};
const struct closure loop_task = {(function_t)Loop_Task};

