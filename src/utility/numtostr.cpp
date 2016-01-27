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

#include <string>
#include <sstream>
#include <assert.h>
#include <stdio.h>
#include "utility/numtostr.h"

using namespace std;

string numtostr_hex( unsigned int value )
{
	char buffer[ 64 ] = { 0 };
	int len = 0;
#if WIN32
	len = sprintf_s( buffer, _countof( buffer ), "%X", value );
#else
	len = sprintf( buffer, "%X", value );
#endif

	if (len >= 0)	return string( buffer, len );
	return string( "" );
}

string numtostr_dec( unsigned int value )
{
	char buffer[ 64 ] = { 0 };
	int len = 0;
#if WIN32
	len = sprintf_s( buffer, _countof( buffer ), "%ul", value );
#else
	len = sprintf( buffer, "%u", value );
#endif

	if (len >= 0)	return string( buffer, len );
	return string( "" );
}

