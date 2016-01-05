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
#include "parse/parser.h"
#include "parse/blanklinefilter.h"
#include "parse/blockstacker.h"
#include "flowgraph/postorderdfs.h"
#include "semantics/semantics.h"
#include "yaml-cpp/emitter.h"

// The output is a YAML stream containing CFGs for the input files.
// Each input file is a separate document.
// Each document is a list of functions.
// A function has a name, an arity, and an expression.
// An expression is a list of operations.
// An operation has an ID, which is unique within the function, and a type,
// which identifies the sort of operation it is; the value is a list of values
// which are operands for the operation.


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

static void emit(Flowgraph::Function *func, YAML::Emitter &dest)
{
	dest << YAML::BeginMap;
	dest << YAML::Key << "name" << YAML::Value << func->Name();
	dest << YAML::Key << "arity" << YAML::Value << func->Arity();
	dest << YAML::Key << "expression" << YAML::Value << YAML::BeginSeq;
	PostOrderDFS nodes(func->Exp());
	std::map<Flowgraph::Node*, unsigned> index;
	while (nodes.Next()) {
		Flowgraph::Node *n = nodes.Current();
		auto ii = index.find(n);
		if (ii != index.end()) {
			dest << YAML::Anchor(std::to_string(ii->second));
		} else {
			dest << YAML::Anchor(std::to_string(index.size()));
			index[n] = index.size();
		}
		dest << n->ToString();
	}
	dest << YAML::EndSeq;
	dest << YAML::EndMap;
}

static int compile(std::string path, std::ostream &dest)
{
	ErrorLog log;
	DummyImporter modules;
	std::ifstream in(path);
	std::stringstream ss;
	ss << in.rdbuf();
	Scanner tokens(ss.str(), path);
	ParserStack ast(tokens, log, path);
	Semantics::Program functions(ast, log, modules, path);
	YAML::Emitter out(dest);
	out << YAML::BeginDoc << YAML::BeginSeq;
	while (functions.Next()) {
		emit(functions.Current(), out);
	}
	out << YAML::EndSeq << YAML::EndDoc;
	return log.HasReceivedReport();
}

int main(int argc, const char *argv[])
{
	// Read source files named in the arguments, compile them, and write the
	// resulting DFGs to stdout as a series of YAML documents.
	if (argc <= 1) {
		std::cerr << "radian-compile: fail: no input files" << std::endl;
		return EXIT_FAILURE;
	}
	bool fail = false;
	for (int i = 1; i < argc; ++i) {
		fail |= compile(argv[i], std::cout);
	}
	return fail? EXIT_FAILURE: EXIT_SUCCESS;
}

