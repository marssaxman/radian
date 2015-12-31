// Copyright 2012 Mars Saxman.
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



#ifndef closure_h
#define closure_h

#include "scope.h"

namespace Semantics {

// Closure is a layer that creates an invokable. It can be captured and called
// from any arbitrary context. Thus it can capture values from its context, but
// it cannot assign to context values.
class Closure : public Layer
{
	public:
		Closure( Flowgraph::Pool &pool, Scope *context );

	protected:
		Flowgraph::Node *Capture( Flowgraph::Node *function );
		void DefineParameters(
                const AST::Expression *params,
                const SourceLocation &loc );
		void DefineOneParameter(
				Flowgraph::Node *symbol,
				Symbol::Type::Enum kind,
				const SourceLocation &loc );
		virtual Symbol::Type::Enum SelfParameterType() const
				{ return Symbol::Type::Def; }

	private:
		Flowgraph::Node *CreateLocalReference(
				Flowgraph::Node *varSym, Flowgraph::Node *value );
		virtual void RebindInContext(
				Flowgraph::Node *sym,
				Flowgraph::Node *value,
				const SourceLocation &loc );
		void ProcessParamList( const AST::Expression *params );
		Flowgraph::Node *_captureList;
		unsigned int _captureCount;
		unsigned int _paramCount;
};

} // namespace Semantics

#endif //closure_h
