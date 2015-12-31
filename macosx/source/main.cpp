// Copyright 2009-2013 Mars Saxman.
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

#include <vector>
using namespace std;
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include "radian.h"

class MacPlatform : public Platform
{
	public:
		int LoadFile( string filepath, string *output );
		string LibDir(const string &execpath);
		string PathSeparator() { return "/"; }
};

int main (int argc, char * const argv[]) 
{
	// We'll just sort of magically assume that all the arguments are in UTF-8.
	deque<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( string( argv[i] ) );
	}

	MacPlatform platform;
	Radian compiler(platform);
	return compiler.Main( args );
}

// MacPlatform::LoadFile
//
// Given a file path, load the contents up as a string. I'd like to change this
// API someday so we can use mmap instead of the file APIs.
//
int MacPlatform::LoadFile( string filepath, string *output )
{
	FILE *input = fopen( filepath.c_str(), "r" );
	if (!input) {
		return errno;
	}
	fseek( input, 0, SEEK_END );
	long length = ftell( input );
	fseek( input, 0, SEEK_SET );
	char *buf = new char[length];
	fread(  buf, sizeof(char), length, input );
	assert( output );
	output->assign( buf, length );
	delete[] buf;
	fclose( input );
	return 0;
}

// MacPlatform::LibDir
//
// Where should we look for the standard object library files?
// 
string MacPlatform::LibDir(const std::string &execpath)
{
	// If there's an environment variable, that will tell us where to find the
	// library. 
	char *libdir = getenv("RADIAN_LIB");
	if (libdir) {
		return string(libdir);
	}
	
	// If there's no environment variable, we will look in the directory that
	// the executable itself lives in.
	return execpath.substr( 0, execpath.find_last_of( '/' ) ) + "/library";
}
