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

#ifndef lex_sourcelocation_h
#define lex_sourcelocation_h

#include <string>

class SourceLocation
{
	public:
        SourceLocation();
		SourceLocation( const SourceLocation &other );
		SourceLocation(
				std::string filepath,
				unsigned int line,
				unsigned int startOffset,
				unsigned int endOffset );
		SourceLocation operator+ ( const SourceLocation &otherLoc ) const;
		std::string ToString() const;
		static SourceLocation File( std::string filepath );
		bool IsDefined() const;
		std::string Dump() const;

	private:
		SourceLocation(
				std::string filepath,
				unsigned int line,
				unsigned int startOffset,
				unsigned int endLine,
				unsigned int endOffset );
		std::string _filePath;
		unsigned int _startLine;
		unsigned int _endLine;
		unsigned int _startOffset;
		unsigned int _endOffset;
};

#endif	//sourcelocation_h
