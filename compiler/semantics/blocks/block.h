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
// Radian.  If not, see <http://www.gnu.org/licenses/>.

#ifndef semantics_blocks_block_h
#define semantics_blocks_block_h

#include "semantics/scope.h"

namespace Semantics {

// A block is a control-flow layer. It does not define an independently
// invokable function: instead its function is dedicated to a single call site.
// The inputs and outputs are determined implicitly, by usage. Statements 
// inside a block can assign new values to variables defined in the block's
// context, because those values form the implicit return value for the block's
// function. A block function cannot be captured or referenced directly: it is
// only invoked implicitly, at the point where it is defined.
class Block : public Layer
{
	typedef Layer inherited;
	public:
		Block( Flowgraph::Pool &pool, Scope *context );
		Scope *Exit( const SourceLocation &loc );
		
	protected:
		virtual void RebindInContext(
				Flowgraph::Node *sym,
				Flowgraph::Node *value,
				const SourceLocation &loc );

		// Block-specific cleanup: must finish up the control structure, then 
		// return a tuple corresponding to the update list. This is presumably
		// created by invoking the block function.
		virtual Flowgraph::Node *ExitBlock( const SourceLocation &loc ) = 0;

		// Subclass must update the rebind list whenever it allows assignment 
		// to some context var.
		Flowgraph::NodeSet _contextRebinds;
		
	private:
		// Once we've gotten the block function results back, we'll redefine
		// context vars using the values in the tuple.
		void MakeContextAssignments(
                Flowgraph::Node *values, const SourceLocation &loc );
};

}	// namespace Semantics

#endif	//block_h
