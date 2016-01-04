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
// Radian.  If not, see <http://www.gnu.org/licenses/>.

#include "semantics/semantics.h"
#include "semantics/scope.h"

using namespace Semantics;

Engine::Engine( 
		Iterator<AST::Statement*> &input, 
		Reporter &log, 
		Importer &modules, 
		const std::string &filepath,
		Scope *root ) :
	_pool(*this, filepath),
	_input(input),
	_errors(log),
	_modules(modules),
	_current(NULL),
	_scope(root),
	_done(false)
{
	assert( root );
}

Program::Program(
		Iterator<AST::Statement*> &input,
		Reporter &log,
		Importer &modules,
		const std::string &filepath ):
	Engine(input, log, modules, filepath, &_globals),
    _filePath(filepath),
	_globals(_pool, log)
{
}

Module::Module(
		Iterator<AST::Statement*> &input,
		Reporter &log,
		Importer &modules,
		const std::string &filepath,
		const std::string &name ):
	Engine(input, log, modules, filepath, &_globals),
    _filePath(filepath),
	_globals(_pool, log, name)
{
}

bool Engine::Next()
{
	// If the queue is empty, process input statements until we have
	// accumulated some functions for output.
	while (_output.empty() && _input.Next()) {
		_scope = _scope->SemGen( _input.Current() );
	}
	
	// If we ran out of input and still don't have any functions to produce,
	// it's time to wrap up this job. Generate the program result; this may add
	// more than one function to our output queue.
	if (_output.empty() && !_done) {
		Exit();
		_done = true;
	}

	if (!_output.empty()) {
		_current = _output.front();
		assert( _current && _current->IsAFunction() );
		_output.pop();
		return true;
	} else {
		return false;
	}
}

Flowgraph::Function *Engine::Current() const
{
	assert( _current && _current->IsAFunction() );
	return _current->AsFunction();
}
		


// Engine::PooledFunction
//
// The pool will call us whenever it caches a new function. Functions, of
// course, are the items we want to yield, so we'll add each newly pooled
// function to our output queue.
//
void Engine::PooledFunction( Flowgraph::Function *it )
{
	assert( it );
	_output.push( it );
}

// Engine::PooledImport
//
// The pool will call us whenever it caches a new import. Imports represent
// other modules this program wants to use. The semantics engine processes a
// single source unit, so we will pass this request up to the compiler driver.
// It will look for the referenced module in the filesystem and presumably
// queue it for compilation.
//
void Engine::PooledImport( Flowgraph::Import *it, const SourceLocation &loc )
{
	std::string fileName = it->FileName()->AsValue()->Contents();
	std::string sourceDir =
			it->SourceDirectory()->IsVoid() ? 
			std::string("") : 
			it->SourceDirectory()->AsValue()->Contents();
	_modules.ImportModule( fileName, sourceDir, loc );
}
