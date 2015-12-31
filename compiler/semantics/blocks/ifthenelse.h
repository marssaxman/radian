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


#ifndef ifthenelse_h
#define ifthenelse_h

#include "block.h"

namespace Semantics {

class IfThenElse : public Block
{
	typedef Block inherited;
	friend class Branch;
	public:
		IfThenElse( Flowgraph::Pool &pool, Scope *context );
		Scope *Enter( const AST::IfThen &it );
		std::string FullyQualifiedName() const;

	protected:
		Flowgraph::Node *CreateLocalReference(
				Flowgraph::Node *sym, Flowgraph::Node *captured );
		Flowgraph::Node *ExitBlock( const SourceLocation &loc );

	private:
		SourceLocation _beginLoc;
		class Branch *_branchChain;
		Flowgraph::NodeMap _parameters;
		Flowgraph::Node *_initialArgs;
		Flowgraph::Node *_forwardingArgs;
		bool _segmented;
		bool _synchronizes;
};

} // namespace Semantics

#endif //ifthenelse_h
