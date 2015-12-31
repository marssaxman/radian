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


#include "program.h"
using namespace Semantics;
using namespace Flowgraph;

// ProgramRoot::ProgramRoot
//
// The program is an async task, and the main function is a task generator.
// That is, the main function returns a task, which the IO loop then runs
// to completion. The final response value will be the program's exit code.
//
ProgramRoot::ProgramRoot( Pool &pool, Reporter &log ):
	Root(pool, log)
{
	// The old monadic io system used a parameter passed to the main function
	// as a representation of world-state, which also offered functions capable
	// of producing new, modified world-state representations. While we are no
	// longer using that system, we'll keep the IO object around for a little
	// while since it is a convenient place to get the IO actions. Once the new
	// async IO system is finished, the IO functions will be broken off into
	// libraries and this implicit "io" parameter will be removed.
	SourceLocation nowhere;
	Define( _pool.Sym_IO(), _pool.Parameter( 0 ), Symbol::Type::Def, nowhere );
	
	// The argument value for the main-level IO sequencer is, of course, the
	// command-line argument vector. 
	Define( _pool.Sym_Argv(), _pool.Parameter( 1 ), Symbol::Type::Def, nowhere );

	// The program has an assert chain, enforcing invariants, which must
	// evaluate true before we can run the IO operation.
	Define( _pool.Sym_Assert(), _pool.True(), Symbol::Type::Var, nowhere );
}

// ProgramRoot::ExitBlock
//
// Capture the result code as the end of the async task chain. If the program
// falls off the end of the source file, we assume it ended normally, so the
// result code will be zero. It must explicitly exit if it wants to return with
// some other result code.
//
Node *ProgramRoot::ExitRoot()
{
	Node *result = _pool.Number( 0 );
	Node *assertChain = Resolve( _pool.Sym_Assert() ).Value();
	result = _pool.Chain( assertChain, result );
	result = PackageSegmentedResult( result );
	return _pool.Function( result, 2 );
}
