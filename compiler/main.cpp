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

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "ast/ast.h"
#include "lex/scanner.h"
#include "lex/commentfilter.h"
#include "lex/whitespacefilter.h"
#include "parse/parser.h"
#include "parse/blanklinefilter.h"
#include "parse/blockstacker.h"
#include "semantics/semantics.h"

// ErrorLog
//
// The parser and semantic analyzer need a destination for error messages. This
// class simply dumps errors to the console.
//
class ErrorLog : public Reporter
{
public:
    ErrorLog() : _empty(true) {}
    void Report(const Error &err)
    {
        std::cerr << err.ToString() << std::endl;
    	_empty = false;
    }
    bool HasReceivedReport() const { return !_empty; }
private:
    bool _empty;
};

// LexerStack
//
// The scanner reads tokens; the comment filter and whitespace filter strip out
// tokens which are not semantically meaningful. The output is an acceptable
// input for the parser stack.
//
class LexerStack : public Iterator<Token>
{
public:
	LexerStack( std::string source, std::string filepath ):
		_scanner(source, filepath),
		_commentFilter(_scanner),
		_whitespaceFilter(_commentFilter) {}
	bool Next() { return _whitespaceFilter.Next(); }
	Token Current() const { return _whitespaceFilter.Current(); }
private:
	Scanner _scanner;
	CommentFilter _commentFilter;
	WhitespaceFilter _whitespaceFilter;
};

// ParserStack
//
// The parser converts a stream of tokens into a stream of statements. The
// blank line filter eliminates lines which have no semantic meaning; the block
// stacker balances begin and end lines, yielding a stream of statements which
// are guaranteed to be semantically balanced. The output is suitable as input
// for the semantic analyzer.
//
class ParserStack : public Iterator<AST::Statement*>
{
public:
	ParserStack(
			Iterator<Token> &input, Reporter &log, std::string filepath ):
		_parser(input, log),
		_blankLineFilter(_parser),
		_blockStacker(_blankLineFilter, log, filepath) {}
	bool Next() { return _blockStacker.Next(); }
	AST::Statement *Current() const { return _blockStacker.Current(); }
private:
	Parser::Engine _parser;
	BlankLineFilter _blankLineFilter;
	BlockStacker _blockStacker;
};

class DummyImporter : public Semantics::Importer
{
public:
	void ImportModule(std::string, std::string, const SourceLocation&) {}
};


int main(int argc, const char *argv[])
{
	// Read source files named in the arguments, compile them, and write the
	// resulting DFGs to stdout as a series of YAML documents.
	if (argc <= 1) {
		std::cerr << "radian-compile: fail: no input files" << std::endl;
		return EXIT_FAILURE;
	}
	ErrorLog log;
	DummyImporter modules;
	for (int i = 1; i < argc; ++i) {
		// compile the file
		std::string path(argv[i]);
		std::ifstream in(path);
		std::stringstream ss;
		ss << in.rdbuf();
		LexerStack tokens(ss.str(), path);
		ParserStack ast(tokens, log, path);
		Semantics::Program functions(ast, log, modules, path);
		// iterate through the sequence of functions, emitting output
	}
	return log.HasReceivedReport()? EXIT_FAILURE: EXIT_SUCCESS;
}
