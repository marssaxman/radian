// Copyright 2013-2014 Mars Saxman
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

#include "bigints.h"
#include "../macros.h"
#include "../buffer.h"
#include "numbers.h"
#include "symbols.h"
#include "rationals.h"
#include "booleans.h"
#include "relations.h"
#include "floats.h"
#include <assert.h>
#include <math.h>
#include <limits.h>

// A bigint is a buffer, an array of unsigned ints, ordered from least to most
// significant. We will never produce a bigint whose buffer is larger than
// necessary; the most significant word will always be non-zero. We handle
// negatives by setting the lowest bit of the lowest word. That is, all bigints
// are effectively shifted left one place, with the sign or'ed in below.

typedef uint32_t digit_t;
#define HIGHBIT ((~0)^((~(digit_t)0)>>1))
#define LOWBIT 1
#define MAX(x,y) ((x>y)?(x):(y))
#define NEGATIVE(x) (BUFDATA((x), digit_t)[0] & LOWBIT)
#define SETNEGATIVE(x) (BUFDATA((x), digit_t)[0] |= LOWBIT)
static value_t Bigint_function( PREFUNC, value_t selector );

struct fakebuf {
	uint8_t bytes[sizeof(struct buffer) + sizeof(digit_t)];
};

static struct twodigitbuf {
	uint8_t bytes[sizeof(struct buffer) + sizeof(digit_t) * 2];
} int_min_buf;
static value_t num_int_min;

void init_bigints( zone_t zone )
{
    // Initialize the special buffer we use to return that one number which is
    // INT_MIN for signed ints, but doesn't fit in a single-digit bigint.
    num_int_min = (value_t)&int_min_buf;
    BUFFER(num_int_min)->function = (function_t)Bigint_function;
    BUFFER(num_int_min)->size = sizeof(digit_t);
    BUFDATA(num_int_min, digit_t)[0] = LOWBIT;
    BUFDATA(num_int_min, digit_t)[1] = 1;
}

static value_t MakeFakeBigint( int value, struct fakebuf *buf )
{
	// Make a fake one-word bigint out of the specified value and return it as
	// though it were an actual object. We populate the fakebuf passed in by
	// the caller, who will presumably perform some computation and return some
	// output value. Note that it is not possible to represent every signed int
	// as a one-word bigint: a bigint is very slightly less efficient, with two
	// representations of zero (positive and negative). In order to store the
	// most negative value which will fit in an int, we would need two digits.
	// Fortunately, we already defined that particular bigint at startup, so we
	// can simply return it whenever we need it, ignoring the fakebuf.
	value_t out = num_zero;
	if ((digit_t)value == HIGHBIT) {
        return num_int_min;
	} else {
		out = (value_t)buf;
		BUFFER(out)->function = (function_t)Bigint_function;
		BUFFER(out)->size = sizeof(digit_t);
		digit_t uvalue = (value < 0) ? ((-value << 1) | LOWBIT) : (value << 1);
		BUFDATA(out, digit_t)[0] = uvalue;
	}
	return out;
}

value_t MakeTemporaryBigint( zone_t zone, int value )
{
    // This is a lot like MakeFakeBigint except we have to actually do a heap
    // alloc. This is for the use of code outside the Bigint module, which
    // can't make assumptions about the structure of a bigint or its lifetime.
	// These one-digit bigints should only be used for temporary intermediate
	// values and should never be released to the wild; any result which can be
	// represented as a fixint should be returned as a fixint instead.
	if ((digit_t)value == HIGHBIT) {
		return num_int_min;
	} else {
		function_t func = (function_t)Bigint_function;
		struct buffer *out = alloc_buffer( zone, func, sizeof(digit_t) );
		digit_t uvalue = (value < 0) ? ((-value << 1) | LOWBIT) : (value << 1);
		*(BUFDATA(out, digit_t)) = uvalue;
		return (value_t)out;
	}
}

static unsigned count_sig_digits( value_t val )
{
	// how many digits does this value have, excluding any most-significant
	// zero digits?
	unsigned size = BUFFER(val)->size / sizeof(digit_t);
	while (size > 0 && 0 == BUFDATA(val, digit_t)[size - 1]) {
		size--;
	}
	return size;
}

static unsigned get_digit( unsigned i, unsigned size, value_t val )
{
	digit_t mask = i > 0 ? ~0 : ~LOWBIT;
	return ((i < size) ? (BUFDATA(val, digit_t)[i]) : 0) & mask;
}

static int unsigned_cmp( value_t left, value_t right )
{
	// Compare the magnitudes of these numbers. Loop through each digit, from
	// least to most significant. If equal, we preserve the previous ordering;
	// otherwise, each difference defines the order between the two ints, until
	// we reach the end.
	unsigned lsize = count_sig_digits(left);
	unsigned rsize = count_sig_digits(right);
	unsigned size = MAX(lsize, rsize);
	int relation = 0;	// equal to
	for (unsigned i = 0; i < size; i++) {
		digit_t l = get_digit(i, lsize, left);
		digit_t r = get_digit(i, rsize, right);
		if (l < r) relation = -1;	// less than
		if (l > r) relation = 1;	// greater than
	}
	return relation;
}

static struct buffer *unsigned_add( zone_t zone, value_t left, value_t right )
{
	// Add two bigint arrays, ignoring their signs. Create an output buffer,
	// set it up as a bigint, and return it. All sizes are in digits. The
	// largest buffer we might need would be one digit larger than the largest
	// operand, in case of rollover. It is NOT legal to cheat and return one of
	// the operands, because the caller expects to get a writable buffer back;
	// it may need to flip the sign.
	unsigned lsize = count_sig_digits(left);
	unsigned rsize = count_sig_digits(right);
	unsigned osize = 1 + MAX(lsize, rsize);
	size_t obytes = osize * sizeof(digit_t);
	struct buffer *out = NULL;
	out = alloc_buffer(zone, (function_t)Bigint_function, obytes);
	// Add the magnitudes of the left and right operands, ignoring their signs,
	// writing the result into the output buffer. We will do this when adding
	// positive numbers, subtracting a positive number from a negative number,
	// or when adding negative numbers.
	bool carry = false;
	for (unsigned i = 0; i < osize; i++) {
		digit_t ldigit = get_digit(i, lsize, left);
		digit_t rdigit = get_digit(i, rsize, right);
		digit_t odigit = ldigit + rdigit + (carry ? 1 : 0);
		BUFDATA(out, digit_t)[i] = odigit;
		carry = (odigit < ldigit) || (odigit < rdigit);
	}
	assert(!carry);
	return out;
}

static struct buffer *unsigned_sub( zone_t zone, value_t left, value_t right )
{
	// Subtract the right value from the left, ignoring their signs. Create an
	// output bigint, populate it, and return it. We assume that left will
	// always be greater than right: the caller must verify that for us. We
	// will always create a buffer as large as the largest value we might need;
	// it's possible that some of the most-significant words will be zeros.
	unsigned lsize = count_sig_digits(left);
	unsigned rsize = count_sig_digits(right);
	unsigned osize = MAX(lsize, rsize);
	size_t obytes = osize * sizeof(digit_t);
	struct buffer *out = NULL;
	out = alloc_buffer(zone, (function_t)Bigint_function, obytes);
	bool borrow = false;
	for (unsigned i = 0; i < osize; i++) {
		digit_t ldigit = get_digit(i, lsize, left);
		digit_t rdigit = get_digit(i, rsize, right);
		if (borrow) {
			rdigit++;
			// if we just overflowed rdigit, we definitely need to carry
			borrow = (0 == rdigit);
		}
		borrow |= rdigit > ldigit;
		BUFDATA(out, digit_t)[i] = ldigit - rdigit;
	}
	return out;
}

static value_t collapse_small_bigints( zone_t zone, value_t value )
{
	// If this bigint is small enough to fit in a fixint, create a fixint and
	// return that instead. We won't return bigints unless we have to.
	unsigned digits = count_sig_digits(value);
	// If the value fits in a single digit, we can definitely return it as a
	// fixint.
	int sign = NEGATIVE(value) ? -1 : 1;
	if (digits <= 1) {
		return NumberFromInt( zone, sign * BUFDATA(value, digit_t)[0] >> 1 );
	}
	// If the value is that one funky negative number which equals INT_MIN,
	// it uses two bigint digits but we can still return it as a fixint.
	if (digits != 2) {
		return value;
	}
	if (BUFDATA(value, digit_t)[0] == 1 && BUFDATA(value, digit_t)[1] == 1) {
		return NumberFromInt( zone, (int)HIGHBIT );
	}
	return value;
}

static value_t Bigint_Compare_to( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		// We don't produce bigints in the range of fixints, so we know the
		// values are not equal, and we know which one has a greater magnitude.
		// All that really matters is the sign of the bigint.
		return NEGATIVE(left) ? LessThan() : GreaterThan();
	} else if (IsARational( right )) {
		value_t numer = RationalNumerator( right );
		value_t denom = RationalNumerator( right );
		left = METHOD_1( left, sym_multiply, denom );
		return METHOD_1( left, sym_compare_to, numer );
	} else if (IsAFloat( right )) {
		double fleft = DoubleFromBigint( left );
		double fright = DoubleFromFloat( right );
		return RelationFromDouble( fleft - fright );
	}
	if (IsABigint( right )) {
		// Check the signs. If they don't match, then whichever is positive is
		// greater and whichever is negative is lesser.
		bool lneg = NEGATIVE(left);
		bool rneg = NEGATIVE(right);
		if (lneg != rneg) {
			return lneg ? LessThan() : GreaterThan();
		}
		int rel = unsigned_cmp(left, right);
		// If both numbers are negative, we must reverse the ordering, since we
		// are comparing absolute magnitudes. The negative number with the
		// greater magnitude is the lesser of the two, and vice versa.
		if (lneg && rneg) {
			rel = -rel;
		}
		return RelationFromInt( rel );
	} else return NaNExp( zone, right );
}

static value_t Bigint_Add( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	struct fakebuf fakebuf;
	if (IsAFixint( right )) {
		right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		left = METHOD_1( left, sym_add, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		return METHOD_1( right, sym_add, left );
	}
	if (IsABigint( right )) {
		bool lneg = NEGATIVE(left);
		bool rneg = NEGATIVE(right);
		value_t out = num_zero;
		if (lneg == rneg) {
			struct buffer *outbuf = unsigned_add( zone, left, right );
			if (lneg) {
				SETNEGATIVE(outbuf);
			}
			out = (value_t)outbuf;
		} else {
			// subtract the operand with the smaller magnitude from the one
			// with the larger magnitude. output gets the sign of the larger
			// operand.
			int rel = unsigned_cmp( left, right );
			if (rel < 0) {
				// Right operand is bigger than left operand.
				struct buffer *outbuf = unsigned_sub( zone, right, left );
				if (rneg) {
					SETNEGATIVE(outbuf);
				}
				out = (value_t)outbuf;
			} else if (rel > 0) {
				// Left operand is bigger than right operand.
				struct buffer *outbuf = unsigned_sub( zone, left, right );
				if (lneg) {
					SETNEGATIVE(outbuf);
				}
				out = (value_t)outbuf;
			}
		}
		if (IsABigint( out )) {
			out = collapse_small_bigints( zone, out );
		}
		return out;
	} else return NaNExp( zone, right );
}

static value_t Bigint_Subtract( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	struct fakebuf fakebuf;
	if (IsAFixint( right )) {
		right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		left = METHOD_1( left, sym_subtract, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		return METHOD_1( right, sym_subtract, left );
	}
	if (IsABigint( right )) {
		bool lneg = NEGATIVE(left);
		bool rneg = NEGATIVE(right);
		value_t out = num_zero;
		// If we have a negative number subtracting a positive, then our result
		// combines their magnitudes, and is negative.
		// If we have a positive number subtracting a negative, then our result
		// combines their magnitudes, and is positive.
		// The rule, then: if signs don't match, add magnitudes and use the
		// sign of the left operand. If signs do match, subtract the magnitude
		// of the smaller from the magnitude of the larger, then invert signs
		// if we had to cross the number line.
		if (lneg != rneg) {
			struct buffer *outbuf = unsigned_add( zone, left, right );
			if (lneg) {
				SETNEGATIVE(outbuf);
			}
			out = (value_t)outbuf;
		} else {
			int rel = unsigned_cmp( left, right );
			if (rel < 0) {
				struct buffer *outbuf = unsigned_sub( zone, right, left );
				if (!lneg) {
					SETNEGATIVE(outbuf);
				}
				out = (value_t)outbuf;
			} else if (rel > 0) {
				struct buffer *outbuf = unsigned_sub( zone, left, right );
				if (lneg) {
					SETNEGATIVE(outbuf);
				}
				out = (value_t)outbuf;
			}
		}
		if (IsABigint( out )) {
			out = collapse_small_bigints( zone, out );
		}
		return out;
	} else return NaNExp( zone, right );
}

static void umul( digit_t left, digit_t right, digit_t *lo, digit_t *hi )
{
	// Perform an unsigned multiplication of two digits. The output will be
	// two more digits, a high and a low. This expects a digit to be 32 bits.
	#define HALFBITS 16
	#define LOWHALFMASK 0x0000FFFF
	#define HIHALFMASK 0xFFFF0000
	// Multiplying two digits produces a two-digit output, which is
	// too large to fit in one digit_t. Split each digit in half
	// and do some half-multiplies, then sum to get our two digits.
	// We'll then sum the digits with the output to get our actual
	// result; this is at w
	// We'll split each digit in half and do some half-multiplies.
	unsigned l_lo = left & LOWHALFMASK;
	unsigned l_hi = (left & HIHALFMASK) >> HALFBITS;
	unsigned r_lo = right & LOWHALFMASK;
	unsigned r_hi = (right & HIHALFMASK) >> HALFBITS;
	digit_t lo_lo = l_lo * r_lo;	// not shifted
	digit_t lo_hi = l_lo * r_hi;	// shifted >> HALFBITS
	digit_t hi_lo = l_hi * r_lo;	// shifted >> HALFBITS
	digit_t hi_hi = l_hi * r_hi;	// shifted >> NUMBITS
	digit_t lo_hi_up = lo_hi << HALFBITS;
	digit_t hi_lo_up = hi_lo << HALFBITS;
	*lo = lo_lo + lo_hi_up + hi_lo_up;
	digit_t carry = (*lo < lo_hi_up || *lo < hi_lo_up) ? 1 : 0;
	*hi = hi_hi + (lo_hi >> HALFBITS) + (hi_lo >> HALFBITS) + carry;
}

static void uadd( digit_t src, digit_t *buf, unsigned size )
{
	// Add one digit to the specified buffer, propagating carry as needed.
	while (size-- > 0 && src > 0) {
		digit_t dest = *buf;
		digit_t sum = dest + src;
		*buf++ = sum;
		src = (sum < dest || sum < dest) ? 1 : 0;
	}
}

static value_t Bigint_Multiply( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	struct fakebuf fakebuf;
	if (IsAFixint( right )) {
		right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, numer );
		return RationalFromIntegers( zone, left, denom );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, DoubleFromBigint( left ) );
		return METHOD_1( left, sym_multiply, right );
	}
	if (IsABigint( right )) {
		// The largest output we might possibly need would be the sum of the
		// sizes of the input values. We might not need all of the space but
		// it is cheaper to allocate it and not use it than to allocate and
		// copy to a new buffer later.
		unsigned lsize = count_sig_digits( left );
		unsigned rsize = count_sig_digits( right );
		unsigned osize = lsize + rsize;
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *out;
		out = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		// Multiply each digit of the left operand by the right operand.
		for (unsigned lindex = 0; lindex < lsize; lindex++) {
			digit_t ldigit = get_digit( lindex, lsize, left );
			// Multiply this digit by each digit of the other operand.
			// Add the result into the destination value.
			for (unsigned rindex = 0; rindex < rsize; rindex++) {
				digit_t rdigit = get_digit( rindex, rsize, right );
				digit_t odigit_lo, odigit_hi;
				umul( ldigit, rdigit, &odigit_lo, &odigit_hi );
				// add the low half of the result to the output, then add the
				// high half of the result to the output, one digit higher.
				unsigned oindex = lindex + rindex;
				digit_t *obuf = &BUFDATA(out, digit_t)[oindex];
				size_t obufsize = osize - oindex;
				uadd( odigit_lo, obuf++, obufsize-- );
				uadd( odigit_hi, obuf++, obufsize-- );
			}
		}
		return collapse_small_bigints( zone, (value_t)out );
	} else return NaNExp( zone, right );
}

static bool div_cmp( value_t denom, struct buffer *remainder, unsigned base )
{
	// Is the portion of the remainder starting at digit base and going up to
	// the end of the buffer greater than or equal to the denominator, or not?
	unsigned lsize = BUFFER(denom)->size / sizeof(digit_t);
	unsigned rsize = remainder->size / sizeof(digit_t);
	unsigned size = MAX(lsize, rsize);
	assert( base < rsize );
	int relation = 0;	// equal to
	for (unsigned i = 0; i < size; i++) {
		digit_t l = get_digit(i, lsize, denom);
		digit_t r = get_digit(i + base, rsize, (value_t)remainder);
		if (l < r) relation = -1;	// less than
		if (l > r) relation = 1;	// greater than
	}
	return relation <= 0;
}

static void div_sub( struct buffer *remainder, value_t denom, unsigned base )
{
	// Subtract the source value from the value in the buffer, writing the
	// result to the buffer. Assumes that the value in the buffer is larger
	// than the value being subtracted. Ignore all digits below i.
	unsigned lsize = (remainder->size) / sizeof(digit_t);
	unsigned rsize = (BUFFER(denom)->size) / sizeof(digit_t);
	bool borrow = false;
	for (unsigned i = 0; i < lsize; i++) {
		digit_t ldigit = get_digit(i + base, lsize, (value_t)remainder);
		digit_t rdigit = get_digit(i, rsize, denom);
		if (borrow) {
			rdigit++;
			// if we just overflowed rdigit, we definitely need to carry
			borrow = (0 == rdigit);
		}
		borrow |= rdigit > ldigit;
		BUFDATA(remainder, digit_t)[i] = ldigit - rdigit;
	}
	assert( !borrow );
}

static void longdiv(
		zone_t zone,
		value_t numer,
		value_t denom,
		value_t *out_quotient,
		value_t *out_remainder)
{
	unsigned nsize = count_sig_digits( numer );
	if (0 == nsize) {
		if (out_quotient) *out_quotient = num_zero;
		if (out_remainder) *out_remainder = num_zero;
		return;
	}
	unsigned dsize = count_sig_digits( denom );
	if (0 == dsize) {
		value_t exception = DivByZeroExp( zone );
		if (out_quotient) *out_quotient = exception;
		if (out_remainder) *out_remainder = exception;
		return;
	}
	unsigned osize = MAX( nsize, dsize );
	size_t obytes = osize * sizeof(digit_t);
	struct buffer *quotient;
	quotient = alloc_buffer( zone, (function_t)Bigint_function, obytes );
	struct buffer *remainder;
	remainder = alloc_buffer( zone, (function_t)Bigint_function, obytes );

	// an incredibly slow algorithm that does in fact work
	for (unsigned digitctr = 1; digitctr <= osize; digitctr++) {
		// Counting from the most to least significant, copy a digit from the
		// numerator into the remainder. While the remainder is greater than
		// the denominator, subtract the denominator from the remainder, adding
		// one to the quotient at each iteration, at the current digit place.
		unsigned i = osize - digitctr;
		BUFDATA(remainder, digit_t)[i] = get_digit(i, nsize, numer);
		while (div_cmp( denom, remainder, i )) {
			div_sub( remainder, denom, i );
			digit_t *quobuf = &(BUFDATA(quotient, digit_t)[i]);
			uadd((i > 0) ? 1 : 2, quobuf, digitctr);
		}
	}

	if (NEGATIVE(numer) != NEGATIVE(denom)) {
		SETNEGATIVE(quotient);
	}
	if (NEGATIVE(numer)) {
		SETNEGATIVE(remainder);
	}

	if (out_quotient) {
		*out_quotient = collapse_small_bigints( zone, (value_t)quotient );
	}
	if (out_remainder) {
		*out_remainder = collapse_small_bigints( zone, (value_t)remainder );
	}
}

static value_t Bigint_Divide( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		// We don't actually divide integers; we just create a rational, using
		// the numerator and the denominator, and let the rational class handle
		// reduction by GCD.
		return RationalFromIntegers( zone, left, right );
	} else if (IsARational( right )) {
		value_t numer = METHOD_0( right, sym_numerator );
		value_t denom = METHOD_0( right, sym_denominator );
		left = METHOD_1( left, sym_multiply, denom );
		return RationalFromIntegers( zone, left, numer );
	} else if (IsAFloat( right )) {
		left = NumberFromDouble( zone, DoubleFromBigint( left ) );
		return METHOD_1( left, sym_divide, right );
	} else return NaNExp( zone, right );
}

static value_t Bigint_Modulus( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		struct fakebuf fakebuf;
		if (IsAFixint( right )) {
			right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
		}
		value_t remainder = NULL;
		longdiv( zone, left, right, NULL, &remainder );
		return remainder;
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_ShiftLeft( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int places = IntFromFixint( right );
		unsigned digits = places / 32;
		unsigned bits = places % 32;
		unsigned flipbits = 32 - bits;
		unsigned lsize = count_sig_digits( left );
		unsigned osize = lsize + digits + 1;
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *outbuf;
		outbuf = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		for (unsigned i = 0; i < lsize; i++) {
			digit_t l = get_digit(i, lsize, left);
			BUFDATA(outbuf, digit_t)[i] |= l >> flipbits;
			BUFDATA(outbuf, digit_t)[i+1] = l << bits;
		}
		if (NEGATIVE( left )) {
			SETNEGATIVE(outbuf);
		}
		return collapse_small_bigints( zone, (value_t)outbuf );
	} else if (IsAnInteger( right )) {
		// Shifting by a bigint is legal, but always shifts all bits away,
		// because any bigint will be bigger than 2^31.
		return num_zero;
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_ShiftRight( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAFixint( right )) {
		int places = IntFromFixint( right );
		unsigned digits = places / 32;
		unsigned bits = places % 32;
		unsigned flipbits = 32 - bits;
		unsigned lsize = count_sig_digits( left );
		unsigned osize = lsize - digits;
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *outbuf;
		outbuf = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		for (unsigned i = 0; i < osize; i++) {
			digit_t l = get_digit(i, lsize, left);
			BUFDATA(outbuf, digit_t)[i] |= l >> bits;
			if (i + 1 < osize) {
				BUFDATA(outbuf, digit_t)[i+1] = l << flipbits;
			}
		}
		if (NEGATIVE( left )) {
			SETNEGATIVE(outbuf);
		}
		return collapse_small_bigints( zone, (value_t)outbuf );
	} else if (IsAnInteger( right )) {
		// Shifting by a bigint is legal but always shifts all bits away.
		return num_zero;
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_BitAnd( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		struct fakebuf fakebuf;
		if (IsAFixint( right )) {
			right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
		}
		unsigned lsize = count_sig_digits( left );
		if (0 == lsize) {
			return num_zero;
		}
		unsigned rsize = count_sig_digits( right );
		if (0 == rsize) {
			return num_zero;
		}
		unsigned osize = MAX( rsize, lsize );
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *outbuf;
		outbuf = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		for (unsigned i = 0; i < osize; i++) {
			digit_t l = get_digit(i, lsize, left);
			digit_t r = get_digit(i, rsize, right);
			BUFDATA(outbuf, digit_t)[i] = l & r;
		}
		return collapse_small_bigints( zone, (value_t)outbuf );
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_BitOr( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		struct fakebuf fakebuf;
		if (IsAFixint( right )) {
			right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
		}
		unsigned lsize = count_sig_digits( left );
		unsigned rsize = count_sig_digits( right );
		unsigned osize = MAX( rsize, lsize );
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *outbuf;
		outbuf = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		for (unsigned i = 0; i < osize; i++) {
			digit_t l = get_digit(i, lsize, left);
			digit_t r = get_digit(i, rsize, right);
			BUFDATA(outbuf, digit_t)[i] = l | r;
		}
		return collapse_small_bigints( zone, (value_t)outbuf );
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_BitXor( PREFUNC, value_t left, value_t right )
{
	ARGCHECK_2( left, right );
	if (IsAnInteger( right )) {
		struct fakebuf fakebuf;
		if (IsAFixint( right )) {
			right = MakeFakeBigint( IntFromFixint( right ), &fakebuf );
		}
		unsigned lsize = count_sig_digits( left );
		unsigned rsize = count_sig_digits( right );
		unsigned osize = MAX( rsize, lsize );
		size_t obytes = osize * sizeof(digit_t);
		struct buffer *outbuf;
		outbuf = alloc_buffer( zone, (function_t)Bigint_function, obytes );
		for (unsigned i = 0; i < osize; i++) {
			digit_t l = get_digit(i, lsize, left);
			digit_t r = get_digit(i, rsize, right);
			BUFDATA(outbuf, digit_t)[i] = l ^ r;
		}
		return collapse_small_bigints( zone, (value_t)outbuf );
	} else if (IsANumber( right )) {
		return ThrowCStr( zone, "non-integer operand in an integer operation" );
	} else return NaNExp( zone, right );
}

static value_t Bigint_Numerator( PREFUNC, value_t value )
{
	ARGCHECK_1(value);
	// Ints are their own numerators.
	return value;
}

static value_t Bigint_Denominator( PREFUNC, value_t value )
{
	ARGCHECK_1(value);
	// Ints all have denominator 1, by definition.
	return num_one;
}

static value_t Bigint_function( PREFUNC, value_t selector )
{
    ARGCHECK_1( selector );
	DEFINE_METHOD(compare_to, Bigint_Compare_to)
	DEFINE_METHOD(add, Bigint_Add)
	DEFINE_METHOD(subtract, Bigint_Subtract)
	DEFINE_METHOD(multiply, Bigint_Multiply)
	DEFINE_METHOD(divide, Bigint_Divide)
	DEFINE_METHOD(modulus, Bigint_Modulus)
//	DEFINE_METHOD(exponentiate, Bigint_Exponentiate)
	DEFINE_METHOD(shift_left, Bigint_ShiftLeft)
	DEFINE_METHOD(shift_right, Bigint_ShiftRight)
	DEFINE_METHOD(bit_and, Bigint_BitAnd)
	DEFINE_METHOD(bit_or, Bigint_BitOr)
	DEFINE_METHOD(bit_xor, Bigint_BitXor)
	DEFINE_METHOD(numerator, Bigint_Numerator)
	DEFINE_METHOD(denominator, Bigint_Denominator)
	if (selector == sym_is_number) return &True_returner;
	if (selector == sym_is_rational) return &True_returner;
	if (selector == sym_is_integer) return &True_returner;
	return ThrowMemberNotFound( zone, selector );
}

bool IsABigint( value_t exp )
{
    return exp && exp->function == (function_t)Bigint_function;
}

double DoubleFromBigint( value_t exp )
{
	// Convert the digits of a bigint into a double. Useful anytime we need to
	// compute a mixed operation, where we must convert to the imprecise type.
	assert( IsABigint( exp ) );
	unsigned esize = BUFFER( exp )->size / sizeof(digit_t);
	double out = BUFDATA( exp, digit_t )[0] >> 1;
	bool negative = NEGATIVE( exp );
	for (unsigned i = 1; i < esize; i++) {
		double fdigit = BUFDATA( exp, digit_t )[i];
		double factor = powf(2.0, (sizeof(digit_t) * 8.0 * (double)i) - 1.0);
		out += fdigit * factor;
	}
	return negative ? -out : out;
}

// BigintQuotient
//
// Divide these bigints and return the quotient, ignoring the remainder.
//
value_t BigintQuotient( zone_t zone, value_t numer, value_t denom )
{
	struct fakebuf fakenumer;
	struct fakebuf fakedenom;
	if (IsAFixint( numer )) {
		numer = MakeFakeBigint( IntFromFixint( numer ), &fakenumer );
	}
	if (IsAFixint( denom )) {
		denom = MakeFakeBigint( IntFromFixint( denom ), &fakedenom );
	}
	assert( IsABigint( numer ) && IsABigint( denom ) );
	value_t quotient = NULL;
	longdiv( zone, numer, denom, &quotient, NULL );
	return quotient;
}


#if RUN_TESTS

// Test helpers
static value_t make_bigint_1( zone_t zone, digit_t dig0 )
{
	function_t function = (function_t)Bigint_function;
	struct buffer *out = alloc_buffer( zone, function, sizeof(digit_t) );
	BUFDATA(out, digit_t)[0] = dig0;
	return (value_t)out;
}

static value_t make_bigint_2( zone_t zone, digit_t dig0, digit_t dig1 )
{
	function_t function = (function_t)Bigint_function;
	struct buffer *out = alloc_buffer( zone, function, sizeof(digit_t) * 2 );
	BUFDATA(out, digit_t)[0] = dig0;
	BUFDATA(out, digit_t)[1] = dig1;
	return (value_t)out;
}

static value_t make_bigint_3( zone_t zone, digit_t d0, digit_t d1, digit_t d2 )
{
	function_t function = (function_t)Bigint_function;
	struct buffer *out = alloc_buffer( zone, function, sizeof(digit_t) * 3 );
	BUFDATA(out, digit_t)[0] = d0;
	BUFDATA(out, digit_t)[1] = d1;
	BUFDATA(out, digit_t)[2] = d2;
	return (value_t)out;
}

static value_t clone_bigint( zone_t zone, value_t bigint )
{
	assert( IsABigint( bigint ) );
	struct buffer *buf = (struct buffer*)bigint;
	return (value_t)clone_buffer( zone, buf->function, buf->size, buf->bytes );
}

static void assert_greater( zone_t zone, value_t l, value_t r )
{
	assert( 1 == IntFromRelation( zone, METHOD_1( l, sym_compare_to, r ) ) );
}

static void assert_equal( zone_t zone, value_t l, value_t r )
{
	assert( 0 == IntFromRelation( zone, METHOD_1( l, sym_compare_to, r ) ) );
}

static void assert_less( zone_t zone, value_t l, value_t r )
{
	assert( -1 == IntFromRelation( zone, METHOD_1( l, sym_compare_to, r ) ) );
}

static void test_bitmacros( zone_t zone )
{
	digit_t x = INT_MIN;
	digit_t y = HIGHBIT;
	assert( x == y );
	assert( x == HIGHBIT );
	assert( INT_MIN == HIGHBIT );
}

static void test_unsigned_cmp( zone_t zone )
{
	// Verify that we can detect bigint equality.
	value_t sam1 = make_bigint_1( zone, 0x12345678 );
	assert( !NEGATIVE(sam1) );
	assert( 0 == unsigned_cmp( sam1, sam1 ) );
	value_t sam2 = make_bigint_1( zone, 0x12345678 );
	assert( 0 == unsigned_cmp( sam1, sam2 ) );
	// Should still be equal even if the number of digits is different, as long
	// as the high digits are all zero.
	value_t sam3 = make_bigint_2( zone, 0x12345678, 0 );
	assert( 0 == unsigned_cmp( sam1, sam3 ) );
	// Low bit indicates sign, but unsigned_cmp is supposed to ignore it,
	// because that's the point of an unsigned comparison.
	SETNEGATIVE(sam2);
	assert( 0 == unsigned_cmp( sam1, sam2 ) );
	value_t sam4 = clone_bigint( zone, sam2 );
	assert( 0 == unsigned_cmp( sam1, sam4 ) );
	assert( 0 == unsigned_cmp( sam2, sam4 ) );

	// Now some orderings. Let's make a big number and a slightly bigger one.
	value_t gt1 = make_bigint_3( zone, 2, 2, 3 );
	value_t gt2 = make_bigint_3( zone, 4, 2, 3 );
	assert( 1 == unsigned_cmp( gt2, gt1 ) );
	// Verify that we get the same result even if one number is negative.
	value_t gt3 = clone_bigint( zone, gt1 );
	SETNEGATIVE(gt3);
	assert( 1 == unsigned_cmp( gt2, gt3 ) );
	// Check that we get the opposite result if we reverse the order.
	assert( -1 == unsigned_cmp( gt3, gt2 ) );
	assert( -1 == unsigned_cmp( gt1, gt2 ) );

	// Let's try an ordering with a differing number of digits.
	value_t dgt1 = make_bigint_3( zone, 0x4040, 2010, 0 );
	value_t dgt2 = make_bigint_2( zone, 0x8081, 16383 );
	assert( 1 == unsigned_cmp( dgt2, dgt1 ) );
}

static void test_unsigned_add( zone_t zone )
{
	// Simplest possible addition
	value_t s1 = MakeTemporaryBigint( zone, 1 );
	value_t s2 = MakeTemporaryBigint( zone, 2 );
	value_t s1s1 = (value_t)unsigned_add( zone, s1, s1 );
	assert( 0 == unsigned_cmp( s2, s1s1 ) );

	// Shouldn't matter if one operand is negative
	value_t s3 = MakeTemporaryBigint( zone, -1 );
	value_t s1s3 = (value_t)unsigned_add( zone, s1, s3 );
	assert( 0 == unsigned_cmp( s2, s1s3 ) );

	// Output should always be positive regardless of operand order
	value_t s3s1 = (value_t)unsigned_add( zone, s3, s1 );
	assert( 0 == unsigned_cmp( s2, s3s1 ) );

	// Now let's get cracking: add some multidigit numbers.
	value_t s4 = make_bigint_2( zone, 0x44444444, 0x55555555 );
	value_t s5 = make_bigint_2( zone, 0x11111111, 0x0000FFFF );
	value_t s4s5 = (value_t)unsigned_add( zone, s4, s5 );
	value_t s6 = make_bigint_2( zone, 0x55555554, 0x55565554 );
	assert( 0 == unsigned_cmp( s6, s4s5 ) );

	// Verify that addition overflows from one digit to the next.
	value_t s7 = make_bigint_2( zone, 0xF0000000, 0x0000000F );
	value_t s8 = make_bigint_1( zone, 0x10000020 );
	value_t s9 = (value_t)unsigned_add( zone, s7, s8 );
	value_t s10 = make_bigint_2( zone, 0x00000020, 0x00000010 );
	assert( 0 == unsigned_cmp( s10, s9 ) );

	value_t s11 = make_bigint_2( zone, 0x00000001, 0xF0000000 );
	value_t s12 = make_bigint_2( zone, 0x00000002, 0x10000000 );
	value_t s13 = (value_t)unsigned_add( zone, s11, s12 );
	value_t s14 = make_bigint_3( zone, 0x00000002, 0x00000000, 0x00000001 );
	assert( 0 == unsigned_cmp( s13, s14 ) );
}

static void test_unsigned_sub( zone_t zone )
{
	// Sign-ignoring subtraction of magnitudes, where left must be greater than
	// right. Output is always positive, regardless of inputs.
	value_t s1 = make_bigint_1( zone, 0x0000000F );
	value_t s2 = make_bigint_1( zone, 0x00000008 );
	value_t s3 = (value_t)unsigned_sub( zone, s1, s2 );
	value_t s4 = make_bigint_1( zone, 0x00000006 );
	assert( 0 == unsigned_cmp( s3, s4 ) );
	assert( 1 == unsigned_cmp( s1, s3 ) );

	// Let's try that with multiple digits.
	value_t s5 = make_bigint_2( zone, 0xFFFFFFFE, 0x00000001 );
	value_t s6 = make_bigint_2( zone, 0, 0x00000001 );
	value_t s7 = (value_t)unsigned_sub( zone, s5, s6 );
	value_t s8 = make_bigint_2( zone, 0xFFFFFFFE, 0 );
	assert( 0 == unsigned_cmp( s7, s8 ) );

	// Let's try it with overflow between digits.
	value_t s9 = make_bigint_2( zone, 0x40000000, 0x00000011 );
	value_t sA = make_bigint_2( zone, 0x90000000, 0x00000001 );
	value_t sB = (value_t)unsigned_sub( zone, s9, sA );
	value_t sC = make_bigint_2( zone, 0xB0000000, 0x0000000F );
	assert( 0 == unsigned_cmp( sB, sC ) );

	// Make sure overflow works properly when a digit rolls over to zero.
	value_t sD = make_bigint_3( zone, 0, 0, 4 );
	value_t sE = make_bigint_3( zone, 0x80000000, 0xFFFFFFFF, 0x00000001 );
	value_t sF = (value_t)unsigned_sub( zone, sD, sE );
	value_t s10 = make_bigint_3( zone, 0x80000000, 0, 2 );
	assert( 0 == unsigned_cmp( s10, sF ) );
}

static void test_add( zone_t zone )
{
	// Add two positive numbers. Verify that the result is positive, that it
	// equals what we expect, and that it is larger than either operand.
	value_t vA = (value_t)make_bigint_2( zone, 0x9ABCDEF0, 0x12345678 );
	value_t vB = (value_t)make_bigint_2( zone, 0x40404040, 0x00004444 );
	value_t vC = METHOD_1( vA, sym_add, vB );
	value_t vD = (value_t)make_bigint_2( zone, 0xDAFD1F30, 0x12349ABC );
	assert_equal( zone, vC, vD );
	assert_greater( zone, vC, vA );
	assert_greater( zone, vC, vB );

	// Add a positive number and a negative number of smaller magnitude. Verify
	// that the result is still positive, that it is larger than zero, and that
	// it equals the value we expect.
	value_t vE = (value_t)make_bigint_2( zone, 0x99998888, 0xCCCCDDDD );
	value_t vF = (value_t)make_bigint_2( zone, 0x11111111, 0x11111111 );
	value_t vG = METHOD_1( vE, sym_add, vF );
	value_t vH = (value_t)make_bigint_2( zone, 0x88887778, 0xBBBBCCCC );
	assert_equal( zone, vH, vG );
	assert_greater( zone, vE, vG );
	assert_greater( zone, vE, vF );
	assert_greater( zone, num_zero, vF );

	// Add a positive number and a negative number of greater magnitude. Verify
	// that the result is negative and that it is greater than the negative
	// operand.
	value_t vI = (value_t)make_bigint_2( zone, 0x99998889, 0xCCCCDDDD );
	value_t vJ = (value_t)make_bigint_2( zone, 0x11111110, 0x11111111 );
	value_t vK = METHOD_1( vI, sym_add, vJ );
	value_t vL = (value_t)make_bigint_2( zone, 0x88887779, 0xBBBBCCCC );
	assert_equal( zone, vL, vK );
	assert_greater( zone, num_zero, vK );

	assert_less( zone, vK, vJ );
	assert_greater( zone, vK, vI );

	// Add a negative number and a positive number of lesser magnitude. Verify
	// that the result is still negative and that the result is what we expect.
	value_t vM = (value_t)make_bigint_3( zone, 1, 0, 0x5555 );
	value_t vN = (value_t)make_bigint_2( zone, 4, 0x999 );
	value_t vO = METHOD_1( vM, sym_add, vN );
	value_t vP = (value_t)make_bigint_3( zone, 0xFFFFFFFD, 0xFFFFF666, 0x5554);
	assert_equal( zone, vP, vO );
	assert_greater( zone, num_zero, vO );
	assert_greater( zone, vO, vM );

	// Add a negative number to a positive number of greater magnitude. Verify
	// that the result is positive and equals the value we expect.
	value_t vQ = (value_t)make_bigint_2( zone, 1, 1 );
	value_t vR = (value_t)make_bigint_2( zone, 0, 4 );
	value_t vS = METHOD_1( vQ, sym_add, vR );
	value_t vT = (value_t)make_bigint_2( zone, 0, 3 );
	assert_equal( zone, vT, vS );
	assert_greater( zone, vS, num_zero );
	assert_greater( zone, vS, vQ );
}

static void test_subtract( zone_t zone )
{
	// Subtract a small positive number from a large positive number. Verify
	// that the result is smaller than the large number, larger than the small
	// number, and positive.
	value_t sA = (value_t)make_bigint_2( zone, 0x9ABCDEF0, 0x12345678 );
	value_t sB = (value_t)make_bigint_2( zone, 0x40404040, 0x00004444 );
	value_t sC = METHOD_1( sA, sym_subtract, sB );
	value_t sD = (value_t)make_bigint_2( zone, 0x5A7C9EB0, 0x12341234 );
	assert_equal( zone, sC, sD );
	assert_greater( zone, sA, sC );
	assert_greater( zone, sC, sB );

	// Subtract a large positive number from a small positive number. Verify
	// that the result is negative and smaller than either operand.
	value_t sE = METHOD_1( sB, sym_subtract, sA );
	value_t sF = (value_t)make_bigint_2( zone, 0x5A7C9EB1, 0x12341234 );
	assert_equal( zone, sF, sE );
	assert_greater( zone, sB, sE );
	assert_greater( zone, sA, sE );

	// Subtract a negative number from itself. Verify that the result is zero.
	value_t sG = METHOD_1( sE, sym_subtract, sF );
	assert_equal( zone, num_zero, sG );

	// Add a negative number to another negative number. Verify that the result
	// is negative and equals what we expect.
	value_t sH = (value_t)make_bigint_2( zone, 1, 0x5555 );
	value_t sI = (value_t)make_bigint_1( zone, 0x88880001 );
	value_t sJ = METHOD_1( sH, sym_add, sI );
	value_t sK = (value_t)make_bigint_2( zone, 0x88880001, 0x5555 );
	assert_equal( zone, sK, sJ );
	assert_greater( zone, sI, sJ );
	assert_greater( zone, sH, sJ );
}

static void test_umul()
{
	// unsigned multiplication: inputs produce double-width output
	digit_t hi, lo;
	umul( 0x12345678, 3, &lo, &hi );
	assert( 0 == hi && 0x369D0368 == lo );

	umul( 0x12345678, 0x444, &lo, &hi );
	assert( 0x4D == hi && 0xA740D7E0 == lo );

	umul( 0xFFFFFFFF, 0xFFFFFFFF, &lo, &hi );
	assert( 0xFFFFFFFE == hi && 0x00000001 == lo );

	umul( 0xb7da82e4, 0x9afb9ec3, &lo, &hi );
	assert( 0x6F4E2800 == hi && 0x65C66BAC == lo );

	umul( 0xeaa88271, 0xbe7f99d1, &lo, &hi );
	assert( 0xAE9E0766 == hi && 0xDD970741 == lo );

	umul( 0x2a7ea096, 0xcc1ca69b, &lo, &hi );
	assert( 0x21E1A978 == hi && 0xEF347ED2 == lo );

	umul( 0x57332f82, 0x390e2a65, &lo, &hi );
	assert( 0x136F38D1 == hi && 0x5819124A == lo );

	umul( 0xa556, 0xf6a6, &lo, &hi );
	assert( 0 == hi && 0x9F4BD9C4 == lo );
}

static void prant( value_t bigint )
{
	size_t digits = BUFFER(bigint)->size / sizeof(digit_t);
	for (unsigned i = 0; i < digits; i++) {
		fprintf(stderr, i > 0 ? " 0x%x" : "0x%x", BUFDATA(bigint, digit_t)[i] );
	}
	fprintf(stderr, "\n");
}

static void test_multiply( zone_t zone )
{
	// Multiply two small positive numbers and make sure the results work out.
	// The result will become a fixint, because it isn't big enough to need
	// more than one digit.
	value_t mA = (value_t)make_bigint_1( zone, 0xa556 );
	value_t mB = (value_t)make_bigint_1( zone, 0xf6a6 );
	value_t mC = METHOD_1( mA, sym_multiply, mB );
	value_t mD = NumberFromInt( zone, 0x4FA5ECE2 );
	assert_equal( zone, mC, mD );

	// Multiply a small positive number by a large number, so the result will
	// overflow into multiple digits.
	value_t mE = (value_t)make_bigint_1( zone, 0xd840 );
	value_t mF = (value_t)make_bigint_1( zone, 0x8FCDBD3A );
	value_t mG = METHOD_1( mE, sym_multiply, mF );
	value_t mH = (value_t)make_bigint_2( zone, 0x8B183E80, 0x7979 );
	assert_equal( zone, mH, mG );

	// Multiply a small negative number by a large number and verify that the
	// result is both large and negative.
	value_t mI = (value_t)make_bigint_1( zone, 0x8fcdbd3a );
	value_t mJ = (value_t)make_bigint_1( zone, 0x9aeb );
	value_t mK = METHOD_1( mI, sym_multiply, mJ );
	value_t mL = (value_t)make_bigint_2( zone, 0x35E3DB04, 0x5705 );
	assert_equal( zone, mK, mL );

	// Multiply a negative number by another negative and verify that the
	// result becomes positive.
	value_t mO = (value_t)make_bigint_2( zone, 0xcbb2f7ad, 0x000779bc );
	value_t mP = (value_t)make_bigint_2( zone, 1, 8 );
	value_t mQ = METHOD_1( mO, sym_multiply, mP );
	value_t mR = (value_t)make_bigint_3( zone, 0, 0x5D97BD60, 0x3BCDE6 );
	assert_equal( zone, mQ, mR );
}

static void test_longdiv( zone_t zone )
{
	// Just about the simplest possible division
	value_t dA = (value_t)make_bigint_1( zone, 8 );
	value_t dB = (value_t)make_bigint_1( zone, 4 );
	value_t dCQ, dCR;
	longdiv( zone, dA, dB, &dCQ, &dCR );
	assert_equal( zone, dCQ, NumberFromInt( zone, 2 ) );
	assert_equal( zone, dCR, num_zero );

	// An identity division; quotient = 1, remainder = 0
	value_t dD = (value_t)make_bigint_2( zone, 0x4598ABF0, 0x003DD21B );
	value_t dEQ, dER;
	longdiv( zone, dD, dD, &dEQ, &dER );
	assert_equal( zone, dEQ, num_one );
	assert_equal( zone, dER, num_zero );

	// Division with a remainder
	value_t dF = (value_t)make_bigint_3( zone, 0x98, 0, 1 );
	value_t dG = (value_t)make_bigint_2( zone, 0, 1 );
	value_t dHQ, dHR;
	longdiv( zone, dF, dG, &dHQ, &dHR );
	value_t dI = (value_t)make_bigint_2( zone, 0, 1 );
	assert_equal( zone, dHQ, dI );
	assert_equal( zone, dHR, NumberFromInt( zone, 0x98 >> 1 ) );

	// Divide small number into large, so numerator becomes remainder
	value_t dJ = (value_t)make_bigint_2( zone, 0xCA314498, 0xB5663166 );
	value_t dK = (value_t)make_bigint_3( zone, 0x9978AAFE, 0x2DC2B28B, 0x4401 );
	value_t dLQ, dLR;
	longdiv( zone, dJ, dK, &dLQ, &dLR );
	assert_equal( zone, dLQ, num_zero );
	assert_equal( zone, dLR, dJ );

	// d4 75 88 dc d2 4e 22 fa
	// 94 1b a3 1d 72 5c 76 8f 7b bd 0d 48 9f 23 0b 69

	// Negative / positive: negative quotient, negative remainder
	value_t dM = (value_t)make_bigint_2( zone, 0xCF0C3F27, 0xEEFD6106 );
	value_t dN = (value_t)make_bigint_2( zone, 0xD47588DC, 3 );
	value_t dOQ, dOR;
	longdiv( zone, dM, dN, &dOQ, &dOR );
	prant( dOQ );
	prant( dOR );
//	assert_equal( zone, dOQ, NumberFromInt( 0x3E669731

	// Positive / negative: negative quotient, positive remainder

	// Negative / negative: positive quotient, negative remainder

}

void test_bigints( zone_t zone )
{
	fprintf( stderr, "bigint tests begin:\n" );
	test_bitmacros( zone );
	test_unsigned_cmp( zone );
	test_unsigned_add( zone );
	test_unsigned_sub( zone );
	test_add( zone );
	test_subtract( zone );
	test_umul();
	test_multiply( zone );
	test_longdiv( zone );
	fprintf( stderr, "...bigint tests done\n" );
}

#endif //RUN_TESTS


