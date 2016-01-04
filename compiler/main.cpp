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

int main ( int argc, char *const argv[] )
{
	// We will assume the args are encoded in UTF-8. There is probably
	// something we should do with the locale setting and iconv, but I haven't
	// worked it out yet.
	deque<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( string( argv[i] ) );
	}

	Radian compiler;
	return compiler.Main( args );
}

