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

int main (int argc, char * const argv[]) 
{
	// We'll just sort of magically assume that all the arguments are in UTF-8.
	deque<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( string( argv[i] ) );
	}

	Radian compiler;
	return compiler.Main( args );
}


