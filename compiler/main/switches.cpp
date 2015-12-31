// Copyright 2012 Mars Saxman.
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


#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include "switches.h"

using namespace std;

// Switches::ProcessArguments
//
// Deal with our argument list. We expect to be invoked with some leading
// switches controlling any compiler options we may feel like exposing,
// followed by a program file name, followed by arguments to be passed on to
// the program we will compile.
//
void Switches::ProcessArguments( deque<string> args )
{
	// We may find some switches controlling compiler behavior.
	while (!args.empty() && ArgIsSwitch(args.front())) {
		string arg = args.front();
		args.pop_front();
		ProcessSwitch( arg );
	}
	if (!_fileArgOmit) {
		// The first argument which is not a compiler switch must be the file
		// path, unless we have processed a switch that makes the file optional.
		AbortIf( args.empty(), "no program file specified", 1 );
		_mainSourceFile = args.front();
		args.pop_front();
		// We will capture any remaining args and save them; they will be the
		// parameters for the program itself, assuming we are going to run it
		// immediately instead of writing it to disk.
		_programArgs.push_back( _mainSourceFile );
		while (!args.empty()) {
			_programArgs.push_back( args.front() );
			args.pop_front();
		}
	}
}

// Switches::ProcessSwitch
//
// Our command-line arguments include some switch. Figure out what it means and
// update our settings so we do the right thing when compiling the project.
// This system is not really finished yet; we don't have any support for the
// unix convention where multiple single-character options can be grouped
// together, and we sdon't have any support for parameterized options.
//
void Switches::ProcessSwitch( string arg )
{
	string name, value;
	ParseSwitch( arg, &name, &value );
	if (name == "o" || name == "output") {
		AbortIf(
				_outputSwitch,
				"only legal to specify one output file name",
				1 );
		_outputSwitch = true;
		_outputSwitchValue = value;
	} else if (name == "c" || name == "compile") {
		ProcessFlagSwitch( _compileOnlySwitch, name, value );
	} else if (name == "dump") {
		AbortIf( value == "",
				"Must supply a dump format. Options are:\n"
				"\ttokens -- token stream, JSON\n"
				"\tflowgraph -- dataflow graph, S-expressions\n"
				"\tlic - linearized intermediate code\n"
				"\tllvm - LLVM IR",
				1 );
		_dumpSwitch = true;
		_dumpSwitchValue = value;
	} else if (name == "target" ) {
		AbortIf( value == "", "must pass in a value for the target triple", 1 );
		_targetTriple = value;
	} else {
		Abort( "illegal option -- " + name, 1 );
	}
}

// Switches::ProcessFlagSwitch
//
// If this switch sets a flag, make sure the flag has not already been set and
// that it was not set with some spurious argument value, then store the value.
//
void Switches::ProcessFlagSwitch( bool &flag, string name, string value )
{
	AbortIf( flag, "'" + name + "' switch specified twice", 1 );
	flag = true;
	AbortIf(
			value.size(), "'" + name +
			"' switch does not accept a value (" + value + ")", 1 );
}

// Switches::ArgIsSwitch
//
// Is this command argument a compiler switch, or a target file? We expect that
// all switch arguments will begin with a distinctive character.
//
bool Switches::ArgIsSwitch( string arg )
{
	return arg.size() > 0 && arg[0] == '-';
}

// Switches::ParseSwitch
//
// Argument syntax is foo=bar, so all we need to do is find the first occurence
// of an equals, then extract the substring to get the name. The remaining text,
// following the equals, is the value. We expect that the shell already handled
// escape characters, so we won't worry about them.
//
void Switches::ParseSwitch( string arg, string *name, string *value )
{
	assert( name && value && ArgIsSwitch( arg ) );
	while (arg.substr( 1, 1 ) == "-") {
		arg = arg.substr( 1, arg.size() - 1 );
	}
	string::size_type separatorpos = arg.find_first_of( '=' );
	if (separatorpos != string::npos) {
		*name = arg.substr( 1, separatorpos - 1 );
		*value = arg.substr( separatorpos + 1 );
	} else {
		*name = arg.substr( 1 );
		*value = string("");
	}
}

// Switches::Abort
//
// Print an error message to stderr and bail out.
//
void Switches::Abort( string error, int code )
{
	cerr << error << endl;
	exit( code );
}

// Switches::AbortIf
//
// Check to see whether some condition has occurred; if so, abort.
//
void Switches::AbortIf( bool condition, string error, int code )
{
	if (condition) Abort( error, code );
}
