// Copyright 2013 Mars Saxman
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
#include "rationals.h"
#include "floats.h"

void DumpToStderr( zone_t zone, value_t data )
{
	if (IsAnException( data )) {
		// we will just blatantly assume we know what the structure of an
		// exception is and print out whatever is in its value slot
		fprintf( stderr, "exception: " );
		DumpToStderr( zone, ExceptionContents( data ) );
	}
	else if (IsAStringLiteral( data )) {
		fprintf( stderr, "%s", CStrFromStringLiteral( data ) );
	}
	else if (IsASymbol( data )) {
		fprintf( stderr, "%s", CStrFromSymbol( data ) );
	}
	else if (IsAFixint( data )) {
		fprintf( stderr, "%d", IntFromFixint( data ) );
	}
	// else if (IsABigint( data )) {
	// fprintf ... what exactly?
	// }
	else if (IsARational( data )) {
		// this won't work if the operands are bigints! fix that
		int numer = IntFromFixint( RationalNumerator( data ) );
		int denom = IntFromFixint( RationalDenominator( data ) );
		fprintf( stderr, "%d/%d", numer, denom );
	}
	else if (IsAFloat( data )) {
		double val = DoubleFromFloat( data );
		fprintf( stderr, "%g", val );
	}
	else if (IsATuple( data )) {
		int size = IntFromFixint( METHOD_0( data, sym_size ) );
		for (int i = 0; i < size; i++) {
			if (i > 0) fprintf( stderr, ", " );
			DumpToStderr( zone, CALL_1( data, NumberFromInt( zone, i ) ) );
		}
	}
	else {
		// Non-literal strings look like normal objects
		const char *str = UnpackString( zone, data );
		if (str) {
			fprintf( stderr, "%s", str );
			free( (void*)str );
		}
		else {
			fprintf( stderr, "<unknown>" );
		}
	}
	// true, false?
	// objects?
}

static value_t debug_trace_func(
		PREFUNC, value_t loc, value_t exp, value_t chain )
{
	ARGCHECK_3( loc, exp, chain );
	fprintf( stderr, "%s: ", CStrFromStringLiteral( loc ) );
	DumpToStderr( zone, exp );
	fprintf( stderr, "\n" );
	return chain;
}

struct closure debug_trace = {(function_t)debug_trace_func};
