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
#include <unistd.h>
#include <deque>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <cstdlib>
#include "main/radian.h"

using namespace std;

class LinuxPlatform : public Platform
{
	public:
		int LoadFile( string filepath, string *output );
		string LibDir(const string &execpath);
		string PathSeparator() { return "/"; }
};

int main ( int argc, char *const argv[] )
{
	// We will assume the args are encoded in UTF-8. There is probably
	// something we should do with the locale setting and iconv, but I haven't
	// worked it out yet.
	deque<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( string( argv[i] ) );
	}

	LinuxPlatform os;
	Radian compiler(os);
	return compiler.Main( args );
}

// LinuxPlatform::LoadFile
//
// Given a file path, load the contents up as a string. We should really use
// mmap instead - this file open and close stuff is pointless and inefficient.
//
int LinuxPlatform::LoadFile( string filepath, string *output )
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

string LinuxPlatform::LibDir(const string &execpath)
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

