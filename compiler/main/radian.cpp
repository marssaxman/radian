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
#include "main/modulelist.h"
#include "main/frontend.h"
#include "main/optionparser.h"

enum  optionIndex { UNKNOWN, HELP, PLUS };
const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "",option::Arg::None, "USAGE: example [options]\n\n"
                                        "Options:" },
 {HELP, 0,"", "help",option::Arg::None, "  --help  \tPrint usage and exit." },
 {PLUS, 0,"p","plus",option::Arg::None, "  --plus, -p  \tIncrement count." },
 {UNKNOWN, 0, "", "",option::Arg::None, "\nExamples:\n"
                               "  example --unknown -- --this_is_no_option\n"
                               "  example -unk --plus -ppp file1 file2\n" },
 {0,0,0,0,0,0}
};

using namespace std;

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

class RunCommand : public Command
{
	public:
		RunCommand( std::string &execpath );
		std::string Description() const
		{
			return "Compile a program and run it";
		}
		std::string Help() const;
		int Run( std::deque<std::string> args );
	private:
        std::string _programRootDir;
		std::string _executablePath;
        std::string _targetTriple;

        void LocateProgramRootDir( std::string mainFile );
		bool CompileProgram( std::string filepath, ModuleList &modules );
		bool CompileModules( ModuleList &modules );
		bool CompileModule( const ModuleRef &item, ModuleList &worklist );
        int LoadFileForTarget(
                const std::string &dirPath,
                const std::string &name,
                std::string *filePath,
                std::string *output);
};

RunCommand::RunCommand( string &execpath ):
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
	string::size_type slashpos = mainfile.find_last_of("/");
	if (slashpos != string::npos) {
		// Break the filename off the source file path to yield the context
		// directory.
		_programRootDir = mainfile.substr( 0, slashpos + 1 );
	} else {
		// Make a reference to the current working directory.
		_programRootDir = "./";
	}
}

static int LoadFile( string filepath, string *output )
{
    FILE *input = fopen( filepath.c_str(), "r" );
    if (!input) {
        return errno;
    }
    fseek( input, 0, SEEK_END );
    long length = ftell( input );
    fseek( input, 0, SEEK_SET );
    char *buf = new char[length];
    fread(  buf, sizeof(char), length, input );
    assert( output );
    output->assign( buf, length );
    delete[] buf;
    fclose( input );
    return 0;
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
	int ioerror = LoadFile( filepath, &source );
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
	string dir = _programRootDir;
	bool allowAccessToBuiltins = false;
	if (item.Directory() == "radian") {
		// If there's an environment variable, that will tell us where to find
		// the library.
		if (char *libvar = getenv("RADIAN_LIB")) {
			dir = string(libvar);
		}
		// If there's no environment variable, we will look in the directory
		// that the executable itself lives in.
		dir = _executablePath.substr(0, _executablePath.find_last_of('/'));
		dir += "/library";
		allowAccessToBuiltins = true;
	} else if (item.Directory().size() > 0) {
		dir += item.Directory();
	}

	// Now that we know where the file lives, go try to open it. If we fail,
	// report an error which is tied to the location that requested the import.
	ErrorLog log;
	string source;
	string filePath;
	std::string name = item.Target();
	int error = LoadFileForTarget( dir, name, &filePath, &source );
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
	std::string base = dirPath + "/" + name;
	int err = 0;
	for (auto suffix: suffixes) {
		*filePath = base + suffix + ".radian";
		int err = LoadFile(*filePath, output);
		if (0 == err) break;
	}
	return err;
}

int Radian::Main( deque<string> args )
{
	// This is the cross-platform entrypoint for the Radian language engine.
	// Figure out what command the user wants to execute, then execute it.

	// Get the zeroth arg, which is always the executable path.
	std::string executablePath = args.front();
	args.pop_front();

	// Build a table of commands we might want to execute.
	map<string, Command*> commands;
	commands["version"] = new VersionCommand;
	Command *run = commands["run"] = new RunCommand( executablePath );
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

int main(int argc, const char *argv[])
{
	// We will assume the args are encoded in UTF-8. There is probably
	// something we should do with the locale setting and iconv, but I haven't
	// worked it out yet.
	deque<string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back( string( argv[i] ) );
	}

	if (argc > 0) {
		// skip the program name when parsing options
		--argc;
		++argv;
	}
	option::Stats stats(usage, argc, argv);
	std::vector<option::Option> options(stats.options_max);
	std::vector<option::Option> buffer(stats.buffer_max);
	option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

	if (parse.error()) return 1;
	if (options[HELP] || argc == 0) {
		option::printUsage(std::cout, usage);
		return 0;
	}

	// Report any errors we discovered while parsing the options.
	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next()) {
		std::cerr << "Unknown option: ";
		std::cerr << std::string(opt->name,opt->namelen) << "\n";
	}
	for (int i = 0; i < parse.nonOptionsCount(); ++i) {
		std::cerr << "Non-option #" << i << ": ";
		std::cerr << parse.nonOption(i) << "\n";
	}

	// Go do useful things.
	Radian compiler;
	return compiler.Main( args );
}

