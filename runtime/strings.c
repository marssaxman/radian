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
#include <stdlib.h>
#include "strings.h"
#include "atoms/numbers.h"
#include "atoms/booleans.h"
#include "atoms/symbols.h"
#include "atoms/fixints.h"
#include "atoms/stringliterals.h"
#include "macros.h"
#include "buffer.h"
#include "exceptions.h"
#include "relations.h"


// Strings are sequences of characters which can be compared or concatenated.
// We will implement a concatenation of two strings as a node in a semi-
// balanced tree. We will try to keep this tree balanced as the user adds more
// strings, so that the cost of traversing a large compound string will grow
// with O(log n) rather than O(n) of the number of components.
//
// Of course it is possible that the program may implement its own string
// concatenation system, invisible to us, and that we'll have trees within
// trees. In that case the balancing act won't work. We'll rely on convenience:
// it'll probably be easier to use the built in concatenator than to write a
// new one, and it'll perform better if you use the built in tool, so perhaps
// people will simply choose to use it instead of writing their own.

#define STRING_CAT_SLOT_COUNT 3
#define STRING_CAT_LEVEL_SLOT 0
#define STRING_CAT_LEFT_SLOT 1
#define STRING_CAT_RIGHT_SLOT 2

#define STRING_CAT_ITER_SLOT_COUNT 2
#define STRING_CAT_ITER_CURRENT_SLOT 0
#define STRING_CAT_ITER_NEXT_SLOT 1

static value_t String_Cat_function( PREFUNC, value_t selector );
static value_t String_Cat_iterator_function( PREFUNC, value_t selector );

static bool IsAStringCat( value_t obj )
{
	return obj && obj->function == (function_t)&String_Cat_function;
}

static value_t String_Cat_iterator_current( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	value_t subiter = iter->slots[STRING_CAT_ITER_CURRENT_SLOT];
	return METHOD_0( subiter, sym_current );
}

static value_t String_Cat_iterator_next( PREFUNC, value_t iter )
{
	ARGCHECK_1( iter );
	value_t current = iter->slots[STRING_CAT_ITER_CURRENT_SLOT];
	value_t next_sequence = iter->slots[STRING_CAT_ITER_NEXT_SLOT];
	current = METHOD_0( current, sym_next );
	value_t is_valid = METHOD_0( current, sym_is_valid );
	if (IsAnException( is_valid )) return is_valid;
	if (BoolFromBoolean( zone, is_valid )) {
		// there's more on the left branch: return another wrapping iterator.
		struct closure *out = ALLOC(
				String_Cat_iterator_function, STRING_CAT_ITER_SLOT_COUNT );
		out->slots[STRING_CAT_ITER_CURRENT_SLOT] = current;
		out->slots[STRING_CAT_ITER_NEXT_SLOT] = next_sequence;
		return (value_t)out;
	}
	else {
		// left branch is done: let the caller iterate over the right branch
		// from now on.
		return METHOD_0( next_sequence, sym_iterate );
	}
}

static value_t String_Cat_iterator_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(current, String_Cat_iterator_current);
	DEFINE_METHOD(next, String_Cat_iterator_next);
	if (sym_is_valid == selector) {
		return &True_returner;
	}
	return ThrowMemberNotFound( zone, selector );
}

static value_t String_Cat_iterate( PREFUNC, value_t str_obj )
{
	ARGCHECK_1( str_obj );
	value_t left = str_obj->slots[STRING_CAT_LEFT_SLOT];
	value_t right = str_obj->slots[STRING_CAT_RIGHT_SLOT];
	// Iteration always proceeds left to right, so crack open the left string
	// first. If it is non-empty, we'll traverse its contents, then seamlessly
	// continue over the right sequence. If it's empty, we'll just return an
	// iterator pointing at the right.
	value_t left_iter = METHOD_0( left, sym_iterate );
	if (IsAnException( left_iter )) return left_iter;
	value_t is_valid = METHOD_0( left_iter, sym_is_valid );
	if (IsAnException( is_valid )) return is_valid;
	if (BoolFromBoolean( zone, is_valid )) {
		// Make a joining iterator that will continue with the right branch.
		struct closure *out = ALLOC(
				String_Cat_iterator_function, STRING_CAT_ITER_SLOT_COUNT );
		out->slots[STRING_CAT_ITER_CURRENT_SLOT] = left_iter;
		out->slots[STRING_CAT_ITER_NEXT_SLOT] = right;
		return (value_t)out;
	}
	else {
		// Left branch is empty, so iterating over the compound string is equal
		// to iterating over the right branch.
		return METHOD_0( right, sym_iterate );
	}
}

static value_t String_Cat_compare_to( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	return CompareStrings( zone, left, right );
}

static value_t String_Cat_concatenate( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	return ConcatStrings( zone, left, right );
}

static value_t String_Cat_function( PREFUNC, value_t selector )
{
	ARGCHECK_1( selector );
	DEFINE_METHOD(iterate, String_Cat_iterate)
	DEFINE_METHOD(compare_to, String_Cat_compare_to)
	DEFINE_METHOD(concatenate, String_Cat_concatenate)
	return ThrowMemberNotFound( zone, selector );
}

// ShouldUseBruteForceCat
//
// Many strings are short, and their performance is dominated by the constant
// overhead factors. Each string literal object has an overhead of at least two
// words, and each cat node uses four. A string concatenated up from single
// characters would thus have an overhead factor of at least 16x on a 32-bit
// machine and 32x on a 64-bit machine - totally uncool. We will thus use a
// crude buffer copy mechanism to concatenate short string literals, producing
// a somewhat less short string literal object; at some point the result will
// grow enough that further concatenation will result in a concat node object.
static bool ShouldUseBruteForceConcat( value_t left, value_t right )
{
	if (!IsAStringLiteral( left )) return false;
	if (!IsAStringLiteral( right )) return false;
	const size_t kMagicArbitraryThreshold = 32;
	// We assume here that string literals are always implemented as buffers.
	// If that assumption ever changed, this code would break.
	if (BUFFER(left)->size >= kMagicArbitraryThreshold) return false;
	if (BUFFER(right)->size >= kMagicArbitraryThreshold) return false;
	// Survived all the checks? Then we'd better use the brute force option.
	return true;
}

static int ConcatNodeLevel( value_t node )
{
	if (IsAStringCat( node )) {
		value_t level_obj = node->slots[STRING_CAT_LEVEL_SLOT];
		return IntFromFixint( level_obj );
	} else {
		return 0;
	}
}

// ConcatStrings
//
// In order to make concatenation cheap, we'll create composite strings by
// joining two existing strings with a concat node.
//
// The naive approach would be to simply create a new concat node for each
// concat operation, using the left operand of the concat operation as the
// left branch of the new tree, and the right operand as the new right branch.
// The problem with this is that concatenation, particularly concatenation
// within a loop, tends to be biased in one direction or the other - typically
// left. 
//
// A left-biased concat tree would be bad news for iteration, because we have
// to create a wrapper iterator for each level we dig in on the leftward side
// of the traversal. We can drop these wrappers as we move to the right branch,
// but if we have a severely left-leaning tree we have to start with a wrapper
// for every component in the composite string - a big waste of memory.
//
// It'd be far better to have a right-biased tree, since then each left branch
// only describes an atomic string, so we only need one wrapper at a time. To
// create a fully right-biased tree, however, we'd have to recurse down to the
// end of the increasingly deep right branch every time we concatenated a new
// element on the right, destroying the performance of the concat operation.
//
// As a compromise, we'll maintain a semi-balanced tree. If you want to concat
// something onto the left side of a composite string, that's fine, go right
// ahead - we don't care how far you bias the tree to the right. If you instead
// concat something onto the right side of a composite string, we'll recurse.
// Before recursing, though, we'll rotate the tree one step if necessary: that
// is, if the right branch is deeper than the left branch, we'll split it up,
// moving the right branch's left over to the left branch's right. We'll then
// continue to append our new chunk onto the remaining stub of the right branch.
//
// This does create one degenerate performance case when you concatenate
// something onto the right side of a string created by repeated leftward
// concatenations. We'll have to rebalance the resulting right-biased tree
// all the way to its tip, looking for a place where we can install the new
// chunk of right-hand text. This is no more than the work we'd have done had
// we built the string left-to-right, though, so the amortized algorithmic
// complexity is the same. If this occasional large rebalance turns out to be
// a problem in practice, we could correct it by introducing rebalancing
// on both leftward and rightward concatenations.
//
value_t ConcatStrings( zone_t zone, value_t left, value_t right )
{
	if (ShouldUseBruteForceConcat( left, right )) {
		return ConcatStringLiterals( zone, left, right );
	}

	if (IsAStringCat( left )) {
		value_t leftleft = left->slots[STRING_CAT_LEFT_SLOT];
		value_t leftright = left->slots[STRING_CAT_RIGHT_SLOT];
		if (ConcatNodeLevel( leftright ) > ConcatNodeLevel( leftleft )) {
			// Right branch is heavier than the left. Time to rebalance.
			value_t leftrightleft = leftright->slots[STRING_CAT_LEFT_SLOT];
			value_t leftrightright = leftright->slots[STRING_CAT_RIGHT_SLOT];
			// Move the left half of the right branch over to the left branch.
			leftleft = ConcatStrings( zone, leftleft, leftrightleft );
			// The remaining bit of right branch is the new right branch, to
			// which we will shortly append some more data.
			leftright = leftrightright;
		}
		// Append the new data to the right branch of the existing cat node.
		// In a moment we'll create a new root node joining the existing left
		// branch of the left operand onto the amended right branch of the left
		// operand, thus creating an entire string.
		left = leftleft;
		right = ConcatStrings( zone, leftright, right );
	}
	// Output is a new node combining the left and the right branches. 
	// Note that we do nothing to prevent trees which are unbalanced rightward:
	// that's totally OK, since iteration proceeds left to right, so we have no
	// additional overhead for the right-biased case. We are not trying to make
	// a balanced tree so much as we are trying to avoid a left-biased one, and
	// to avoid such a tree without imposing an undue cost on the concatenation
	// operation.
	int leftlevel = ConcatNodeLevel( left );
	int rightlevel = ConcatNodeLevel( right );
	int newlevel = 1 + (leftlevel ? (leftlevel > rightlevel) : rightlevel);
	struct closure *out = ALLOC( &String_Cat_function, STRING_CAT_SLOT_COUNT );
	out->slots[STRING_CAT_LEVEL_SLOT] = NumberFromInt( zone, newlevel );
	out->slots[STRING_CAT_LEFT_SLOT] = left;
	out->slots[STRING_CAT_RIGHT_SLOT] = right;
	return (value_t)out;
}

value_t CompareStrings( zone_t zone, value_t left, value_t right )
{
	// Discover an ordering relationship between these strings.
	// Generic string function: must not assume its arguments are atomic string
	// literals.
	// Simple ordinal comparison. Users must handle issues of normalization and
	// case folding themselves. This just compares codepoints. Useful for
	// storing strings in maps, among other things.
	value_t l_iter = METHOD_0( left, sym_iterate );
	value_t r_iter = METHOD_0( right, sym_iterate );

	while (true) {
		value_t l_is_valid_obj = METHOD_0( l_iter, sym_is_valid );
		if (IsAnException( l_is_valid_obj )) return l_is_valid_obj;
		value_t r_is_valid_obj = METHOD_0( r_iter, sym_is_valid );
		if (IsAnException( r_is_valid_obj )) return r_is_valid_obj;
		bool l_valid = BoolFromBoolean( zone, l_is_valid_obj );
		bool r_valid = BoolFromBoolean( zone, r_is_valid_obj );

		if (l_valid && r_valid) {
			value_t l_char_obj = METHOD_0( l_iter, sym_current );
			if (IsAnException( l_char_obj )) return l_char_obj;
			if (!IsAFixint( l_char_obj )) {
				return ThrowCStr( zone, "left operand is not a string" );
			}
			value_t r_char_obj = METHOD_0( r_iter, sym_current );
			if (IsAnException( r_char_obj )) return r_char_obj;
			if (!IsAFixint( r_char_obj )) {
				return ThrowCStr( zone, "right operand is not a string" );
			}

			int l_char = IntFromFixint( l_char_obj );
			int r_char = IntFromFixint( r_char_obj );
			if (l_char < r_char) return LessThan();
			if (l_char > r_char) return GreaterThan();

			l_iter = METHOD_0( l_iter, sym_next );
			r_iter = METHOD_0( r_iter, sym_next );
		} else {
			if (r_valid && !l_valid) return LessThan();
			if (l_valid && !r_valid) return GreaterThan();
			break;
		}
	}
	return EqualTo();
}

static void BufUp( char **buffer, size_t *size, unsigned endex )
{
	// We are about to write some more bytes into this buffer, starting at
	// index 'i'. Make sure the buffer is large enough to hold those bytes.
	// If the buffer is too small, we will allocate a larger one.
	if (endex >= *size) {
		*size *= 2;
		if (endex >= *size) {
			*size = endex + 1;
		}
		*buffer = realloc( *buffer, *size );
	}
}

const char *UnpackString( zone_t zone, value_t it )
{
	if (!it) return NULL;
	value_t iter = METHOD_0( it, sym_iterate );
	if (IsAnException( iter )) return NULL;

	// Create the initial buffer, just large enough to hold the terminator.
	// We will resize this unless we find no characters.
	size_t size = 1;
	char *buffer = malloc(size);
	unsigned i = 0;

	while (BoolFromBoolean( zone, METHOD_0( iter, sym_is_valid ) )) {
		value_t current = METHOD_0( iter, sym_current );
		if (IsAnException( iter )) goto error;
		if (!IsAFixint( current )) goto error;

		// Unpack the current character into the output buffer, reallocating
		// it if necessary to ensure that we have enough space.
		int ch = IntFromFixint( current );
		if (ch <= 0x00007F) {
			BufUp( &buffer, &size, i + 1 );
			buffer[i++] = (char)(ch & 0xFF);
		} else if (ch >= 0x000080 && ch <= 0x0007FF) {
			BufUp( &buffer, &size, i + 2 );
			buffer[i++] = (0xC0 | ((ch >> 6) & 0x1F));
			buffer[i++] = (0x80 | (ch & 0x3F));
		} else if (ch >= 0x000800 && ch <= 0x00FFFF) {
			BufUp( &buffer, &size, i + 3 );
			buffer[i++] = (0xE0 | ((ch >> 12) & 0x0F));
			buffer[i++] = (0x80 | ((ch >> 6) & 0x3F));
			buffer[i++] = (0x80 | ((ch & 0x3F)));
		} else if (ch >= 0x010000 && ch <= 0x10FFFF) {
			BufUp( &buffer, &size, i + 4 );
			buffer[i++] = (0xF0 | ((ch >> 18) & 0x07));
			buffer[i++] = (0x80 | ((ch >> 12) & 0x3F));
			buffer[i++] = (0x80 | ((ch >> 6) & 0x3F));
			buffer[i++] = (0x80 | (ch & 0x3F));
		} else {
			goto error;
		}

		iter = METHOD_0( iter, sym_next );
		if (IsAnException( iter )) goto error;
	}

	// Write a terminator into the buffer and return it.
	BufUp( &buffer, &size, i + 1 );
	buffer[i++] = '\0';
	return buffer;

error:
	free( buffer );
	return NULL;
}
