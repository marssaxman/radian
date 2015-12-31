// Copyright 2012-2013 Mars Saxman.
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


#include "dumpcommand.h"
#include "emitllvm.h"
#include "frontend.h"
#include "modulelist.h"
#include "linearizer.h"
#include "postorderdfs.h"
#include <iostream>

using namespace std;

DumpCommand::DumpCommand(Platform &os) : _os(os) {}

string DumpCommand::Description() const
{
	return "Compile and print out intermediate forms";
}

string DumpCommand::Help() const
{
	string out =
	"usage: radian dump <format> [--target=x] <file>\n"
	"\n"
	"Compile a program until the compiler has produced intermediate data in \n"
	"the specified format, then print the result to standard output.\n"
	"\n"
	"Formats include:\n"
	"    tokens         token stream, JSON\n"
	"    flowgraph      dataflow graph, S-expressions\n"
	"    lic            linearized intermediate code\n"
	"    llvm           LLVM IR, building for the chosen target\n"
	"\n"
	"For LLVM, you may specify a target triple via the --target switch.\n"
	"If you don't specify a --target, the compiler will use the default.\n"
	"";
	return out;
}

static int DumpTokens( string path, string sourceFile )
{
    LexerStack input( sourceFile, path );
    while (input.Next()) {
        cout << input.Current().Dump() << endl;
    }
	return 0;
}

static int DumpFlowgraph( string path, string sourceFile )
{
	ErrorLog log;
	LexerStack lexer( sourceFile, path );
	ParserStack input( lexer, log, sourceFile );
	ModuleList modules;
	Semantics::Program semantics( input, log, modules, path );
	while (semantics.Next()) {
		cout << semantics.Current()->ToString() << endl;
	}
	return 0;
}

static int DumpLIC( string path, string sourceFile )
{
	ErrorLog log;
	LexerStack lexer( sourceFile, path );
	ParserStack input( lexer, log, sourceFile );
	ModuleList modules;
	Semantics::Program semantics( input, log, modules, path );
	while (semantics.Next()) {
		cout << Linearizer::ToString( semantics.Current() ) << endl;
	}
	return 0;
}

static int DumpIR( string path, string sourceFile, string triple )
{
	ErrorLog log;
	LexerStack lexer( sourceFile, path );
	ParserStack input( lexer, log, sourceFile );
	ModuleList modules;
	Semantics::Program sp( input, log, modules, path );
	EmitLLVM::Program emitter( sp );
	
	llvm::LLVMContext context;
	llvm::Module *module = emitter.Run( context, path, triple );
	module->dump();
	delete module;
	
	return 0;
}

static int Dump( string path, string source, string format, string triple )
{
	if (format == "tokens") return DumpTokens( path, source );
	if (format == "flowgraph") return DumpFlowgraph( path, source );
	if (format == "lic") return DumpLIC( path, source );
	if (format == "llvm") return DumpIR( path, source, triple );
	cerr << "Unknown output dump format \"" << format << "\"" << std::endl;
	cerr << "Known formats are:" << std::endl;
	cerr << "\ttokens -- token stream, JSON" << std::endl;
	cerr << "\tflowgraph -- dataflow graph, S-expressions" << std::endl;
	cerr << "\tlic - linearized intermediate code" << std::endl;
	cerr << "\tllvm - LLVM IR" << std::endl;
	return 1;
}

int DumpCommand::Run( deque<string> args )
{
	string format;
	if (!args.empty()) {
		format = args.front();
		args.pop_front();
	}

	// la la la, we only have one optional argument
	std::string target;
	if (!args.empty()) {
		std::string targetstr = "--target=";
		std::string arg = args.front();
		if (arg.substr(0, targetstr.size()) == targetstr) {
			target = arg.substr(targetstr.size(), string::npos);
			args.pop_front();
		}
	}

	std::string path;
	if (!args.empty()) {
		path = args.front();
		args.pop_front();
	} else {
		cerr << "Error: Must specify a target file" << endl;
		cout << Help() << endl;
		return 1;
	}

	if (!args.empty()) {
		cerr << "Unusable argument: " << args.front() << endl;
		cout << Help() << endl;
		return 1;
	}

	std::string file;
	int ioerror = _os.LoadFile( path, &file );
	if (ioerror != 0) {
		ErrorLog log;
		log.ReportError(
				Error::Type::LoadProgramFileFailed,
				SourceLocation::File( path ) );
		return ioerror;
	}

	return Dump( path, file, format, target );
}

