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

#include "semantics/blocks/block.h"

using namespace Semantics;
using namespace Flowgraph;

Block::Block( Flowgraph::Pool &pool, Scope *context ) :
		Layer(pool, context) 
{
}

Scope *Block::Exit( const SourceLocation &loc )
{
	Node *values = ExitBlock( loc );
	MakeContextAssignments( values, loc );
	return inherited::Exit( loc );
}

void Block::RebindInContext(
		Node *sym, Node *value, const SourceLocation &loc )
{
	_contextRebinds.insert( sym );
}

void Block::MakeContextAssignments( Node *values, const SourceLocation &loc )
{
	// Iterate through the list of rebinds. Look up the tuple value for each
	// one and then assign it to the corresponding symbol in the context.
	unsigned int index = 0;
	for (auto sym: _contextRebinds) {
		Node *indexNumber = _pool.Number( index++ );
		Node *newValue = _pool.Call1( values, indexNumber );
		// We believe that all of these symbols should be assignable; we won't
		// have put them on the rebind list otherwise. Therefore, it is not
		// necessary to supply a source location, which is only used for error
		// reporting; and that's fortunate as we don't actually have a
		// sourcelocation right now.
		Context().Assign( sym, newValue, loc );
	}
}

