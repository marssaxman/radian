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


#include "booleans.h"
#include "macros.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

static value_t True_function( PREFUNC, value_t trueval, value_t falseval )
{
	return trueval;
}
static struct closure True = {(function_t)True_function};

static value_t False_function( PREFUNC, value_t trueval, value_t falseval )
{
	return falseval;
}
static struct closure False = {(function_t)False_function};

value_t BooleanFromBool( bool data )
{
	return data ? &True : &False;
}

bool BoolFromBoolean( zone_t zone, value_t it )
{
	return CALL_2( it, it, NULL) == it;
}

static value_t True_returner_function( PREFUNC, value_t obj )
{
	return &True;
}
struct closure True_returner = {(function_t)True_returner_function};

static value_t False_returner_function( PREFUNC, value_t obj )
{
	return &False;
}
struct closure False_returner = {(function_t)False_returner_function};



