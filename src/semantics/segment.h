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

// Segments contain pieces of a scope which can be executed asynchronously.
// Each segment is a separate invokable within a single semantic scope. Symbol
// definitions span segments. A scope may have only one segment.

#ifndef semantics_segment_h
#define semantics_segment_h

#include <map>
#include <queue>
#include "flowgraph/pool.h"
#include "semantics/symbols.h"
#include "flowgraph/node.h"

namespace Semantics {

class Segment
{
	public:
		struct Type { enum Enum {
			Yield,
			YieldFrom,
			Sync
		}; };

		Segment(
				Flowgraph::Pool &pool,
				Segment *previous,
				const SymbolTable &symbols,
				Flowgraph::Node *value,
				Type::Enum type );
		~Segment();
		Symbol Resolve( Flowgraph::Node *name );
		void PropagateCapturedValue( Flowgraph::Node *name, Symbol sym );
		Flowgraph::Node *Iterator( Flowgraph::Node *next );
		void RewriteCapturedValues( Flowgraph::NodeMap &reMap );
		bool Synchronization() const { return _type == Type::Sync; }

	protected:
		Flowgraph::Node *Sym_Make_Next() const;
		Flowgraph::Node *Sym_Begin() const;
		Flowgraph::Node *Sym_Make_Sub() const;

	private:
		Flowgraph::Pool &_pool;
		Segment *_previous;
		SymbolTable _symbols;
		Flowgraph::Node *_value;
		Type::Enum _type;
		SymbolTable _nextSegmentSlotRefs;
		std::queue<Flowgraph::Node*> _nextSegmentCaptures;
};

} // namespace Semantics

#endif // segment_h
