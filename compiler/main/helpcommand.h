// Copyright 2013-2016 Mars Saxman.
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

#ifndef main_helpcommand_h
#define main_helpcommand_h

#include <map>
#include "main/command.h"

class HelpCommand : public Command
{
	public:
		HelpCommand(std::map<std::string, Command*> &commands);
		std::string Description() const;
		std::string Help() const;
		int Run( std::deque<std::string> args );
	private:
		std::map<std::string, Command*> &_commands;
};

#endif // helpcommand_h
