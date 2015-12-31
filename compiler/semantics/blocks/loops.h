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



#ifndef loops_h
#define loops_h

#include "block.h"
#include <map>

namespace Semantics {

class Loop : public Block
{
	typedef Block inherited;
	protected:
		Loop( Flowgraph::Pool &pool, Scope *context );
		virtual void RebindInContext(
				Flowgraph::Node *name,
				Flowgraph::Node *value,
				const SourceLocation &loc );
		Flowgraph::Node *ExitBlock( const SourceLocation &loc );
		Flowgraph::Node *CreateLocalReference(
				Flowgraph::Node *name, Flowgraph::Node *value );
		void SetConditionExpression( Flowgraph::Node *exp );
		virtual const SourceLocation &BeginLoc() = 0;

        // Methods the ForLoop subclass may override if it likes
        virtual Flowgraph::Node *GenerateLoopOperation(
                Flowgraph::Node *argTuple,
                Flowgraph::Node *condition,
                Flowgraph::Node *operation );
        virtual void ApplyParameterMapping(
                Flowgraph::Node *key,
                Flowgraph::Node *oldValue,
                Flowgraph::Node *newValue ) {}

	private:
		Flowgraph::Node *OperationFunction( Flowgraph::Node *captures );
		Flowgraph::Node *ConditionFunction( Flowgraph::Node *captures );		
		Flowgraph::Node *StartArgs() const;
		Flowgraph::Node *RemapIO( const SourceLocation &loc );

		// Expression to be wrapped up as the condition function, set by the
		// subclass during its Enter action.
		Flowgraph::Node *_condition;
		// Symbols that we have read from, but not assigned to, and are thus
		// invariant through the loop.
		Flowgraph::NodeMap _invariantList;
		// Symbols we have assigned to, that we ought to rebind in our context.
		Flowgraph::NodeMap _updateList;
		// Map of input values for each symbol.
		Flowgraph::NodeMap _startValues;
		// Ticker used to generate placeholders.
		unsigned _placeholderIndex;
};

class WhileLoop : public Loop
{
	public:
		WhileLoop( Flowgraph::Pool &pool, Scope *context );
		void Enter( const AST::While &it );
		std::string FullyQualifiedName() const;

	protected:
		const SourceLocation &BeginLoc() { return _beginLoc; }

	private:
		SourceLocation _beginLoc;

};

} // namespace Semantics

#endif //loops_h
