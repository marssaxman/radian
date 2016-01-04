// Copyright 2012-2016 Mars Saxman.
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


#ifndef main_switches_h
#define main_switches_h

#include <deque>
#include <vector>
#include <string>
#include <stdbool.h>

class Switches
{
	public:
		Switches() :
			_outputSwitch(false),
			_dumpSwitch(false),
			_compileOnlySwitch(false),
			_fileArgOmit(false) {}
		virtual ~Switches() {}
		// Invoke the compiler by instantiating it and passing in the command
		// line args, as strings in UTF-8.
		void ProcessArguments( std::deque<std::string> args );

		std::string MainFile() const { return _mainSourceFile; }
		bool Output() const { return _outputSwitch; }
		std::string OutputValue() const { return _outputSwitchValue; }
		bool Dump() const { return _dumpSwitch; }
		std::string DumpValue() const { return _dumpSwitchValue; }
		bool CompileOnly() const { return _compileOnlySwitch; }
		bool FileArgOmitted() const { return _fileArgOmit; }
		std::string TargetTriple() const { return _targetTriple; }
		const std::vector<std::string> &ProgramArgs() const
				{ return _programArgs; }

	protected:
       // We have failed. Print an error message to the standard error log and
        // abort with the given code.
        virtual void Abort( std::string error, int code );
        void AbortIf( bool condition, std::string error, int code );

		// Given one of the argument strings passed in to Run, is this a
		// compiler switch or a file name? The default implementation assumes
		// that switches can be identified by a leading character, which
		// SwitchChar() will return.
		virtual bool ArgIsSwitch( std::string arg );
		// Given an argument which is known to be a compiler switch, unpack the
		// name and value strings. We expect that the argument name will be a
		// simple identifier, followed by a colon, and the remaining text - if
		// present - is the value.
		void ParseSwitch(
				std::string arg,
				std::string *name,
				std::string *value );
		// Processing our arguments populates the configuration variables. This
		// is the first step in any run. The compiler instance is not reentrant
		// (can only be run once at a time).
		void ProcessSwitch( std::string arg );
		void ProcessFlagSwitch(
				bool &flag,
				std::string name,
				std::string value );

	private:
		// What source code file are we compiling? This is not optional.
		std::string _mainSourceFile;
		bool _outputSwitch;
		std::string _outputSwitchValue;
		bool _dumpSwitch;
		std::string _dumpSwitchValue;
		bool _compileOnlySwitch;
		bool _fileArgOmit;
		std::string _targetTriple;
		// If there are any remaining arguments, we will pass them along to the
		// program when we run it.
		std::vector<std::string> _programArgs;
};

#endif // switches_h
