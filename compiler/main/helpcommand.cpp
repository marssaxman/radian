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

#include <iostream>
#include "main/helpcommand.h"

HelpCommand::HelpCommand(std::map<std::string, Command*> &commands):
	_commands(commands)
{
}

std::string HelpCommand::Description() const
{
	return "Show information about commands";
}

std::string HelpCommand::Help() const
{
	std::string out = 
		"Radian is a programming language for multicore computing.\n"
		"\n"
		"Usage: radian command [arguments]\n"
		"\n"
		"The commands are:\n";

	// We're going to do some pretty formatting, where we line up the columns
	// for the description strings, and for that we need to know how long the
	// longest command name is.
	unsigned longestName = 0;
	for (auto cmdpair: _commands) {
		unsigned nameLen = cmdpair.first.size();
		if (nameLen > longestName) {
			longestName = nameLen;
		}
	}
	unsigned tabLen = (longestName + 5) & ~(3);

	// Go through the commands. Print each one's name and description.
	for (auto cmdpair: _commands) {
		std::string name = cmdpair.first;
		Command *cmd = cmdpair.second;
		while (name.size() < tabLen) {
			name.push_back(' ');
		}
		std::string line = "    " + name;
		line += cmd->Description() + "\n";
		out += line;
	}
	return out + "\n"
			"Use \"radian help [command]\" for more information "
			"about a command.";
}

int HelpCommand::Run( std::deque<std::string> args )
{
	std::string help;
	if (args.empty()) {
		help = Help();
	} else {
		std::string name = args.front();
		args.pop_front();
		auto pair = _commands.find(name);
		if (pair != _commands.end()) {
			help = pair->second->Help();
		} else {
			help = "Unknown help topic `" + name + "`.  Run 'radian help'.";
		}
	}
	std::cerr << help << std::endl;
	return 0;
}
