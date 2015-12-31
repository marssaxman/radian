// Copyright 2009-2012 Mars Saxman.
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


#ifndef semantics_h
#define semantics_h

#include "sequence.h"
#include "flowgraph.h"
#include "statementanalyzer.h"
#include "program.h"
#include "module.h"
#include "ast.h"
#include <queue>
#include <string>

namespace Semantics {

class Importer
{
	public:
		virtual ~Importer() {}
		virtual void ImportModule(
				std::string identifier,
				std::string source,
				const SourceLocation &loc ) = 0;
};

class Engine : public Iterator<Flowgraph::Function*>, public Flowgraph::Delegate
{
	public:
		Engine( 
			Iterator<AST::Statement*> &input, 
			Reporter &log, 
			Importer &modules, 
			const std::string &filepath,
			Scope *root );
		virtual ~Engine() {}
		bool Next();
		Flowgraph::Function *Current() const;
		
	protected:
		// Implementation of Flowgraph::Delegate
		void PooledFunction( Flowgraph::Function *it );
		void PooledImport( Flowgraph::Import *it, const SourceLocation &loc );
		virtual void Exit() = 0;
		Flowgraph::Pool _pool;

	private:
		Iterator<AST::Statement*> &_input;
		Reporter &_errors;
		Importer &_modules;
		std::queue<Flowgraph::Node*> _output;
		Flowgraph::Node *_current;
		Scope *_scope;
		bool _done;
};

class Program : public Engine
{
	public:
		Program(
			Iterator<AST::Statement*> &input,
			Reporter &log,
			Importer &modules,
			const std::string &filepath );
	protected:
		void Exit() { _globals.Exit( SourceLocation::File( _filePath ) ); }
	private:
		std::string _filePath;
		ProgramRoot _globals;
};

class Module : public Engine
{
	public:
		Module(
			Iterator<AST::Statement*> &input,
			Reporter &log,
			Importer &modules,
			const std::string &filepath,
			const std::string &name );
		void EnableBuiltins(void) { _globals.EnableBuiltins(); }
	protected:
		void Exit() { _globals.Exit( SourceLocation::File( _filePath ) ); }
	private:
		std::string _filePath;
		ModuleRoot _globals;
};

}	// namespace Semantics

#endif //semantics_h
