// Copyright 2009-2012 Mars Saxman.
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
#include <algorithm>
#include <limits.h>
#include "sourcelocation.h"
#include "numtostr.h"

using namespace std;

SourceLocation::SourceLocation(
		std::string filepath,
		unsigned int line,
		unsigned int startOffset,
		unsigned int endOffset)
{
	_filePath = filepath;
	_startLine = line;
	_endLine = line;
	_startOffset = startOffset;
	_endOffset = endOffset;
	assert( endOffset >= startOffset );
	if (line > 0 || startOffset > 0 || endOffset > 0) {
		assert( filepath.size() > 0 );
	}
}

SourceLocation::SourceLocation(
		std::string filepath,
		unsigned int startLine,
		unsigned int startOffset,
		unsigned int endLine,
		unsigned int endOffset)
{
	_filePath = filepath;
    _startLine = startLine;
	_endLine = endLine;
	_startOffset = startOffset;
	_endOffset = endOffset;
	assert( 
		endLine > startLine || 
		(endLine == startLine && endOffset >= startOffset) );
}

SourceLocation::SourceLocation(const SourceLocation &other)
{
	_filePath = other._filePath;
	_startLine = other._startLine;
	_endLine = other._endLine;
	_startOffset = other._startOffset;
	_endOffset = other._endOffset;
}

SourceLocation SourceLocation::operator+ (const SourceLocation &otherLoc) const
{
	bool thisdefined = IsDefined();
	bool otherdefined = otherLoc.IsDefined();
	assert( thisdefined && otherdefined );
	assert( _filePath == otherLoc._filePath );
	int outStartLine, outEndLine = 0;
	int outStartOffset, outEndOffset = 0;

	if (_startLine < otherLoc._startLine) {
		// Self starts on a line prior to the other location, so we'll use its
		// char offset.
		outStartLine = _startLine;
		outStartOffset = _startOffset;
	} else if (_startLine > otherLoc._startLine) {
		// The other location starts on a line prior to self, so we'll use its
		// char offset.
		outStartLine = otherLoc._startLine;
		outStartOffset = otherLoc._startOffset;
	} else {
		// Both locations start on the same line. We'll use the smaller of the
		// char offsets.
		outStartLine = _startLine;
		outStartOffset = min(_startOffset, otherLoc._startOffset);
	}

	if (_endLine > otherLoc._endLine) {
		// Self finishes later than the other location, so we'll use its line
		// offset.
		outEndLine = _endLine;
		outEndOffset = _endOffset;
	} else if (_endLine < otherLoc._endLine) {
		// The other location finishes later, so we'll use its offset.
		outEndLine = otherLoc._endLine;
		outEndOffset = otherLoc._endOffset;
	} else {
		// Both locations end on the same line. We'll use the greater char
		// offset.
		outEndLine = _endLine;
		outEndOffset = max(_startOffset, otherLoc._endOffset);
	}

	SourceLocation out(
			_filePath, outStartLine, outStartOffset, outEndLine, outEndOffset);
	return out;
}

// SourceLocation::ToString
//
// Print this source location in text form, suitable for an error message or
// other human-readable output. We will use three formats, depending on the
// range of text the location encloses:
// If the location encloses no text, we return L (C) (line & char)
// If the location exists within a line, we return L (CB-CE)
// If the location spans multiple lines, we return LB-LE
//
// We will bow to decades of misguided convention and pretend that source lines
// start counting at 1 instead of 0, since virtually all text editors use one-
// based line numbering and it would otherwise be unnecessarily difficult to
// match error messages to the lines from which they arose.
//
string SourceLocation::ToString() const
{
	const unsigned kludge = 1;
	if (!IsDefined()) return "";
	string out = _filePath;
	if (_startLine == _endLine) {
		out += ", line " + numtostr_dec( _startLine + kludge );
		out += "(";
		out += numtostr_dec( _startOffset );
		if (_startOffset != _endOffset) {
			out += "-" + numtostr_dec( _endOffset );
		}
		out += ")";
	} else {
		out += ", lines ";
		out += numtostr_dec( _startLine + kludge ) + "-" +
				numtostr_dec( _endLine + kludge );
	}
	return out;
}

// SourceLocation::SourceLocation
//
// Default state is a location that cannot possibly exist, where the start line
// & column are UINT_MAX and the ends are zero. It is impossible to construct
// such a sourcelocation explicitly; it only ever exists as an uninitialized
// object, but is at least identifiable as such.
//
SourceLocation::SourceLocation()
{
	_filePath = "";
	_startLine = UINT_MAX;
    _startOffset = UINT_MAX;
    _endLine = 0;
    _endOffset = 0;
    assert (!IsDefined());
}

// SourceLocation::File
//
// Return a location which encompasses a whole file. This is a lot like the
// Nowhere location, but it includes a file path. Instead of setting the start
// line and offset to UINT_MAX, they are equal to zero, and the end line is
// equal to UINT_MAX, suggesting that we include an infinite number of lines.
//
SourceLocation SourceLocation::File( std::string filepath )
{
    SourceLocation out( filepath, 0, 0, UINT_MAX, 0 );
    return out;
}

// SourceLocation::Defined
//
// Is this a real location, or is it the nowhere location?
//
bool SourceLocation::IsDefined() const
{
	return !(_startLine == UINT_MAX && _startOffset == UINT_MAX);
}

string SourceLocation::Dump() const
{
	if (!IsDefined()) return "{nowhere}";
	// Matching the kludge in ToString, we will print line numbers that begin
	// counting with one instead of zero.
	const unsigned kludge = 1;
	string out = "{";
	out += "filepath: \"" + _filePath + "\", ";
	out += "start: {";
	out += "line: " + numtostr_dec( _startLine + kludge ) + ", ";
	out += "offset: " + numtostr_dec( _startOffset );
	out += "}, end: {";
	out += "line: " + numtostr_dec( _endLine + kludge ) + ", ";
	out += "offset: " + numtostr_dec( _endOffset );
	out += "}}";
	return out;
}
