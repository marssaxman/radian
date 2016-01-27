// Copyright 2009-2016 Mars Saxman.
//
// Radian is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 2 of the License, or (at your option) any later
// version.
//
// Radian is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// Radian.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include <ctype.h>
#include "unicode/unicode.h"

// This file will probably break up into smaller pieces someday.
// We will eventually implement the logic described in "UAX #31, Unicode
// Identifier And Pattern Syntax".
//		<http://www.unicode.org/reports/tr31/>

// We report errors by returning U+FFFD, "REPLACEMENT CHARACTER".
// See the Unicode FAQ:
//	<http://unicode.org/unicode/faq/utf_bom.html#gen8>
static const uchar_t kErrorChar = 0x0000FFFD;

Unicode::Iterator::Iterator( std::string src ) :
	_source(src),
	_offsetBytes(0)
{
}

void Unicode::Iterator::MoveTo( unsigned int pos )
{
	assert( pos <= _source.size() );
	_offsetBytes = pos;
}

// Unicode::Iterator::GuessEncoding
//
// Create a new input iterator, based on a guess about the input string's
// encoding. We expect that the input string represents the entire contents of
// a text file, and that this text file is encoded in either UTF-8 or UTF-16.	 
// The first few bytes of the input string will tell us what encoding we are
// dealing with. We expect that a UTF-16 file will begin with a byte-order mark,
// U+FEFF. If we see such a character we will read the rest of the file as
// UTF-16-BE. If instead we see the invalid character U+FFFE, we know that the
// file must have been written on a little-endian system, so we will swap pairs
// of bytes and read the stream as UTF-16-LE. If we do not see either of these
// byte sequences, we will read the file as UTF-8. We do not recognize any
// other text encodings. You must delete the returned iterator object when you
// are done with it.
//
Unicode::Iterator *Unicode::Iterator::GuessEncoding( std::string source )
{
	if (source.size() >= 2 &&
			(unsigned char)source[0] == 0xFE &&
			(unsigned char)source[1] == 0xFF) {
		return new UTF16BE::Iterator( source );
	} else if (source.size() >= 2 &&
			(unsigned char)source[0] == 0xFF &&
			(unsigned char)source[1] == 0xFE) {
		return new UTF16LE::Iterator( source );
	} else {
		return new UTF8::Iterator( source );
	}
}

// UTF8::Iterator::Next
//
// Get the next character from this UTF-8 byte stream. If we got a character,
// we will return it. If we were not able to get a character, we will return
// the invalid character U+FFFD, 'REPLACEMENT CHARACTER'.
//
uchar_t UTF8::Iterator::Next()
{
	// Get characters until we reach the end of the string; then return false.
	if (_offsetBytes >= _source.size()) {
		return kErrorChar;
	}
	// The leading byte of a UTF-8 character encodes the length of the
	// character's representation in its high bits. The leading byte will also
	// identify trailing octets; if we discover such a byte, it means that the
	// input is corrupt. In the normal case, we will decode the remaining bytes
	// and return the result as a single unicode character.
	unsigned char ch = _source[_offsetBytes];
	if ((ch & 0xC0) == 0x80) {
		_offsetBytes++;
		return kErrorChar;
	}
	unsigned int len = 1;
	if (0xC0 == (ch & 0xC0)) {
		len = 2;
		if (0xE0 == (ch & 0xE0)) {
			len = 3;
			if (0xF0 == (ch & 0xF0)) {
				len = 4;
			}
		}
	}
	// Now that we know the length of the whole character's representation in
	// bytes, we can shift in the remaining bits to get the actual code point
	// value.
	if (_offsetBytes + len <= _source.size()) {
		unsigned int out = 0;
		switch (len) {
			case 1: {
				out = ch;
			} break;
			case 2: {
				out = ((ch & 0x1F) << 6) | 
					(_source[_offsetBytes + 1] & 0x3F);
				if (out < 0x000080) {	// overlong sequence
					out = kErrorChar;
				}
			} break;
			case 3: {
				out = ((ch & 0x0F) << 12) |
					(_source[_offsetBytes + 1] & 0x3F) << 6 |
					(_source[_offsetBytes + 2] & 0x3F);
				if (out < 0x000800) {	// overlong sequence
					out = kErrorChar;
				}
			} break;
			case 4: {
				out = ((ch & 0x07) << 18) |
					(_source[_offsetBytes + 1] & 0x3F) << 12 |
					(_source[_offsetBytes + 2] & 0x3F) << 6 |
					(_source[_offsetBytes + 3] & 0x3F);
				if (out < 0x010000) {	// overlong sequence
					out = kErrorChar;
				}
			} break;
			default: assert(false);
		}
		_offsetBytes += len;
		return out;
	} else {
		// Error case: incomplete character sequence.
		_offsetBytes = _source.size();
		return kErrorChar;
	}
}

// UTF8::Writer::Append
//
// Dump this character out to our destination string in UTF-8 format. 
//
void UTF8::Writer::Append( uchar_t ch )
{
	assert( ch <= 0x10FFFF );
	if (ch <= 0x00007F) {
		_value.append( 1, (char)(ch & 0xFF) );
	} else if (ch >= 0x000080 && ch <= 0x0007FF) {
		_value.append( 1, 0xC0 | ((ch >> 6) & 0x1F) );
		_value.append( 1, 0x80 | (ch & 0x3F) );
	} else if (ch >= 0x000800 && ch <= 0x00FFFF) {
		_value.append( 1, 0xE0 | ((ch >> 12) & 0x0F) );
		_value.append( 1, 0x80 | ((ch >> 6) & 0x3F) );
		_value.append( 1, 0x80 | ((ch & 0x3F)) );
	} else if (ch >= 0x010000 && ch <= 0x10FFFF) {
		_value.append( 1, 0xF0 | ((ch >> 18) & 0x07) );
		_value.append( 1, 0x80 | ((ch >> 12) & 0x3F) );
		_value.append( 1, 0x80 | ((ch >> 6) & 0x3F) );
		_value.append( 1, 0x80 | (ch & 0x3F) );
	}
}

// UTF16::Iterator::Next
//
// Return the next character from this UTF-16 code stream. This will usually
// just be the next byte pair, but we must watch out for surrogate pairs, too.
//
uchar_t UTF16::Iterator::Next()
{
	if (_offsetBytes + 2 > _source.size()) {
		// Error case: incomplete character sequence.
		_offsetBytes = _source.size();
		return kErrorChar;
	}
	unsigned int firstpair = Next16Bits();
	_offsetBytes += 2;
	// Is this 16-bit value a basic-plane character, a leading surrogate, or a
	// trailing surrogate?
	if (firstpair >= 0xD800 && firstpair <= 0xDBFF) {
		// This is a leading surrogate. We should attempt to get the trailing
		// surrogate and combine values.
		if (_offsetBytes + 2 > _source.size()) {
			// Error case: incomplete character sequence.
			return kErrorChar;
		}
		unsigned int secondpair = Next16Bits();
		_offsetBytes += 2;
		// Combine the pairs according to the algorithm in the Unicode FAQ: 
		//		<http://unicode.org/faq/utf_bom.html#utf16-3>
		return ((((firstpair >> 6) & 0x001F) + 1) << 16) |
				(((firstpair & 0x003F) << 10) | (secondpair & 0x03FF));
	} else if (firstpair >= 0xDC00 && firstpair <= 0xDFFF) {
		// This is a trailing surrogate. We should not encounter such a
		// character on its own; this is an error.
		return kErrorChar;
	} else {
		// This is a normal, single 16-bit code unit.
		return firstpair;
	}
}

// UTF16::Writer::Append
//
// Write this character out to our destination in UTF-16. The subclass will
// deal with endianness.
// 
void UTF16::Writer::Append( uchar_t ch )
{
	assert( ch <= 0x10FFFF );
	if (ch <= 0xFFFF) {
		// BMP char, one code unit, stands for itself.
		Next16Bits( ch );
	} else {
		// Non-BMP char, crack it into a surrogate pair.
		ch -= 0x10000;
		Next16Bits( 0xD800 | ((ch >> 10) & 0x03FF) );
		Next16Bits( 0xDC00 | (ch & 0x03FF) );
	}
}

// UTF16BE::Iterator::Next16Bits
//
// Get the next 16-bit code unit from the byte stream, in big-endian format.
unsigned int UTF16BE::Iterator::Next16Bits()
{
	return (((unsigned char)_source[_offsetBytes] & 0x00FF) << 8) |
			((unsigned char)_source[_offsetBytes + 1] & 0x00FF);
}

// UTF16BE::Writer::Next16Bits
//
// Write this 16-bit code unit to the byte stream, in big-endian (MSB-first)
// format. No safety checking, because this can only be invoked from
// UTF16::Writer::Append above, which we trust to invoke its helper function
// appropriately.
//
void UTF16BE::Writer::Next16Bits( unsigned int val )
{
	_value.append( 1, (val >> 8) & 0xFF );
	_value.append( 1, val & 0xFF );
}

// UTF16LE::Iterator::Next16Bits
//
// Get the next 16-bit code unit from the byte stream, in little-endian format.
unsigned int UTF16LE::Iterator::Next16Bits()
{
	return (((unsigned char)_source[_offsetBytes + 1] & 0x00FF) << 8) |
			((unsigned char)_source[_offsetBytes] & 0x00FF);
}

// UTF16LE::Writer::Next16Bits
//
// Write this 16-bit code unit to the byte stream, in little-endian (LSB-first)
// format.
//
void UTF16LE::Writer::Next16Bits( unsigned int val )
{
	_value.append( 1, val & 0xFF );
	_value.append( 1, (val >> 8) & 0xFF );
}

