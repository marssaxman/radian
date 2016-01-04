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
// Radian. If not, see <http://www.gnu.org/licenses/>.


// We use the traditional three-part major/minor/revision numbering scheme.
// - If a new release contains changes which may break backward compatibility,
//	increment the major version number and reset the minor and revision to zero.
// - If a new release contains changes which may break forward compatibility,
//	increment the minor version number and reset the revision number to zero.
// - If a new release contains only bug fixes, performance improvements, or
// other changes which do not affect compatibility,
//	increment the revision number.
#define VERSION "0.7.0"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <list>
#include "main/radian.h"
#include "main/dumpcommand.h"
#include "main/helpcommand.h"
#include "main/runcommand.h"

using namespace std;

class VersionCommand : public Command
{
	std::string Description() const
	{
		return "Print version number";
	}
	std::string Help() const
	{
		return "usage: radian version\n\n"
				"Version prints the Radian version number.";
	}
	int Run( std::deque<std::string> args )
	{
		cout << VERSION << endl;
		return 0;
	}
};

int Radian::Main( deque<string> args )
{
	// This is the cross-platform entrypoint for the Radian language engine.
	// Figure out what command the user wants to execute, then execute it.

	// Get the zeroth arg, which is always the executable path.
	std::string executablePath = args.front();
	args.pop_front();

	// Build a table of commands we might want to execute.
	map<string, Command*> commands;
	Command *help = commands["help"] = new HelpCommand( commands );
	commands["version"] = new VersionCommand;
	commands["dump"] = new DumpCommand;
	Command *run = commands["run"] = new RunCommand( executablePath );
	
	// If there is no argument, execute the help command.
	if (args.empty()) {
		return help->Run( args );
	}
	
	// Get the first arg, which might be a command name.
	// If it is a command name, execute it.
	// If it is not a command name, execute the default command, "run".
	auto commap = commands.find( args.front() );
	if (commap != commands.end()) {
		args.pop_front();
		return commap->second->Run( args );
	} else {
		return run->Run( args );
	}
}

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

