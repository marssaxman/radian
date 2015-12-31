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

#ifndef command_h
#define command_h

#include <string>
#include <deque>

class Command
{
	public:
		virtual ~Command() {}
		// Short sentence explaining what the command is for
		virtual std::string Description() const = 0;
		// Page of text explaining how to use the command
		virtual std::string Help() const = 0;
		// Actually do whatever the command is for
		virtual int Run( std::deque<std::string> args ) = 0;
};

#endif
