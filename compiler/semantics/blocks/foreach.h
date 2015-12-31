// Copyright 2009-2013 Mars Saxman.
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


#ifndef foreach_h
#define foreach_h

#include "loops.h"

namespace Semantics {

class ForLoop : public Loop
{
	typedef Loop inherited;
	public:
		ForLoop( Flowgraph::Pool &pool, Scope *context );
		void Enter( const AST::ForLoop &it );
		std::string FullyQualifiedName() const;

	protected:
		const SourceLocation &BeginLoc() { return _beginLoc; }
        void ApplyParameterMapping(
                Flowgraph::Node *key,
                Flowgraph::Node *oldValue,
                Flowgraph::Node *newValue );
        Flowgraph::Node *GenerateLoopOperation(
                Flowgraph::Node *argTuple,
                Flowgraph::Node *condition,
                Flowgraph::Node *operation );
        void FindMappableSubexpressions(
                Flowgraph::Node *operation,
                Flowgraph::NodeSet *mappables );
        Flowgraph::Node *Parallelize(
                Flowgraph::Node *argTuple, Flowgraph::Node *inputSequence);
        Flowgraph::Node *GenerateMapper(
                const Flowgraph::NodeSet &mappables,
                Flowgraph::Node *captures);

	private:
		SourceLocation _beginLoc;
        // Values in our context
        Flowgraph::Node *_sequence;
        Flowgraph::Node *_contextIterator;
        // Values inside the loop body
        Flowgraph::Node *_iteratorName;
        Flowgraph::Node *_localIterator;

};

} // namespace Semantics

#endif //foreach_h
