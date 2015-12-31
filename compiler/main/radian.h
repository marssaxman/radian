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


#ifndef radian_h
#define radian_h

#include <deque>
#include <string>
#include "platform.h"

class Radian
{
	public:
		Radian(Platform &platform);
		virtual ~Radian() {}
		// Invoke the compiler by instantiating it and passing in the command
		// line args, which must be encoded in UTF-8.
		int Main( std::deque<std::string> args );
	private:
		Platform &_os;
};

#endif // radian_h
