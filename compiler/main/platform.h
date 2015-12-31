// Copyright 2013 Mars Saxman.
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
// Radian. If not, see <http://www.gnu.org/licenses/>.

#ifndef platform_h
#define platform_h

// Abstraction describing all operating system services the compiler uses.
class Platform
{
	public:
		virtual ~Platform() {}
		// Unless otherwise specified, all string parameters are encoded in
		// UTF-8 and all returned strings must be encoded in UTF-8.
		// Given a file path, load its contents. The filepath will be encoded
		// in UTF-8 as usual, but the file contents should be returned as a
		// simple byte array, with no encoding transformation. Return zero if
        // the load operation failed, otherwise return some error code. If the
        // load failed, do not change the contents of the output string.
		virtual int LoadFile( std::string filepath, std::string *output ) = 0;
		// What is the path to the directory containing the standard library?
		virtual std::string LibDir(const std::string &execpath) = 0;
		// Which character separates paths on this platform?
		virtual std::string PathSeparator() = 0;
};


#endif	// platform_h
