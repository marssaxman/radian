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
#include <fstream>
#include <stdlib.h>
#include <list>
#include "main/radian.h"
#include "main/frontend.h"
#include "semantics/semantics.h"
#include "semantics/symbols.h"
#include "linearcode/linearizer.h"
#include "main/runcommand.h"
#include "main/switches.h"

using namespace std;

RunCommand::RunCommand( Platform &os, string &execpath ):
	_os(os),
	_executablePath(execpath)
{
}

std::string RunCommand::Help() const
{
	std::string out =
"usage: radian run <file> [arguments...]\n"
"\n"
"Run compiles the specified file as a program and runs it.\n"
"Any additional arguments, following the program file, will be passed along\n"
"to the running program as its arguments.\n"
"";
	return out;
}

int RunCommand::Run( deque<string> args )
{
	// Process the argument list and find the program file we need to run.
	Switches switches;
	switches.ProcessArguments( args );
	_targetTriple = switches.TargetTriple();
	std::string mainFile = switches.MainFile();
	LocateProgramRootDir( mainFile );

	// Compile the program file, generating a list of modules it imported.
	ModuleList worklist;
	if (CompileProgram( mainFile, worklist )) return true;

	// Compile the modules the main program imported. These modules may import
	// additional modules, which we must also compile, until we have compiled
	// the transitive closure of all modules referenced by the main program.
	if (CompileModules( worklist )) return true;

	fprintf(stderr, "To do: implement a backend.\n");
	exit(1);
}

// RunCommand::LocateProgramRootDir
//
// After we've parsed the command line arguments, we can use the name of the
// main program file to determine which directory is the root. We'll use that
// root directory to import other modules.
//
void RunCommand::LocateProgramRootDir(string mainfile)
{
	// We will look for these modules in the directory containing the main
	// source file.
	string separator = _os.PathSeparator();
	string::size_type slashpos = mainfile.find_last_of( separator );
	if (slashpos != string::npos) {
		// Break the filename off the source file path to yield the context
		// directory.
		_programRootDir = mainfile.substr( 0, slashpos + 1 );
	} else {
		// Make a reference to the current working directory.
		_programRootDir = "." + separator;
	}
}

// RunCommand::CompileProgram
//
// Compile the main source file of this program out to an object file. We will
// collect references to other modules as we go so we can later compile them
// too. Returns true if it found an error, false if compilation succeeded.
//
bool RunCommand::CompileProgram( string filepath, ModuleList &modules )
{
	ErrorLog log;
	string source;
	int ioerror = _os.LoadFile( filepath, &source );
	if (ioerror) {
		log.ReportError(
				Error::Type::LoadProgramFileFailed,
				SourceLocation::File( filepath ) );
		return true;
	}
	LexerStack lexer(source, filepath);
	ParserStack input(lexer, log, filepath);
	Semantics::Program sp( input, log, modules, filepath );

	// oops, we need to actually implement a compiler
	return true;
}

// RunCommand::CompileModules
//
// The main source file may depend on some other modules, which may have their
// own dependencies. Compile modules until we have built everything we need to
// link the finished executable.
//
bool RunCommand::CompileModules( ModuleList &worklist )
{
	bool error = false;
	set<string> compiled;
	while (!worklist.Empty()) {
		// Get the next module from our work list. See whether we've already
		// compiled it. If so, there's no need to do it again; otherwise, we'll
		// compile it and add it to the list of compiled modules.
		ModuleRef item = worklist.Front();
		worklist.Pop();
		if (compiled.count( item.UniqueID() )) continue;
		compiled.insert( item.UniqueID() );
		bool compileError = CompileModule( item, worklist );
		if (compileError) error = true;
	}
	return error;
}

// RunCommand::CompileModule
//
// Compile a suport module out to an object file. We will collect references to
// other modules as we go, since modules may depend on each other.
//
bool RunCommand::CompileModule( const ModuleRef &item, ModuleList &modules )
{
	// Load the source code for the module. Locate the directory this module
	// lives in. Check for our special "radian" directory, which represents the
	// standard library. Only the standard library gets access to the "builtin"
	// functions provided by the C-implemented runtime library.
	string moduleDir = _programRootDir;
	bool allowAccessToBuiltins = false;
	if (item.Directory() == "radian") {
		moduleDir = _os.LibDir(_executablePath);
		allowAccessToBuiltins = true;
	} else if (item.Directory().size() > 0) {
		moduleDir += item.Directory();
	}

	// Now that we know where the file lives, go try to open it. If we fail,
	// report an error which is tied to the location that requested the import.
	ErrorLog log;
	string source;
	string filePath;
	std::string name = item.Target();
	int error = LoadFileForTarget( moduleDir, name, &filePath, &source );
	if (error != 0) {
		log.ReportError( Error::Type::ImportFailed, item.SourceLoc() );
		return true;
	}

	// Now that we have loaded the source code, set up a compiler and run it.
	LexerStack lexer(source, filePath);
	ParserStack input(lexer, log, filePath);
	Semantics::Module sm( input, log, modules, filePath, name );
	if (allowAccessToBuiltins) sm.EnableBuiltins();

	// Er, right. We need a backend here.
	return true;

	// Push the compiled module onto our output list. When all modules have
	// been compiled, we'll pass this list into the linker.
	//_compiledModules.push_back( module );
	//return log.HasReceivedReport();
}

// RunCommand::LoadFileForTarget
//
// File name suffixes denote target-specific versions of a module. A module may
// be specialized for the operating system, processor architecture, or both.
// We will first look for a version specialized for both, then for one which is
// specialized for the operating system, then one which is specialized for the
// processor architecture, and if we find none of these we will attempt to
// load the basic version which is applicable to all targets, which will be the
// most common case.
//
int RunCommand::LoadFileForTarget(
		const std::string &dirPath,
		const std::string &name,
		std::string *filePath,
		std::string *output)
{
	assert(filePath);
	assert(output);
	output->clear();
	std::string varArch = "implement me please";
    std::string varOS = "unknown";

	std::list<std::string> suffixes;
	suffixes.push_back("-" + varOS + "-" + varArch);
	suffixes.push_back("-" + varOS);
	suffixes.push_back("-" + varArch);
	suffixes.push_back("");
	std::string base = dirPath + _os.PathSeparator() + name;
	int err = 0;
	for (auto suffix: suffixes) {
		*filePath = base + suffix + ".radian";
		int err = _os.LoadFile(*filePath, output);
		if (0 == err) break;
	}
	return err;
}

