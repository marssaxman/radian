// Copyright 2010-2011 Mars Saxman.
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

#ifndef memberdispatch_h
#define memberdispatch_h

#include "flowgraph.h"
#include "scope.h"
#include <set>
#include <vector>

namespace Semantics {

class MemberDispatch
{
	public:
		MemberDispatch( Flowgraph::Pool &pool );
		void SetPrototype( Flowgraph::Node *prototype );
		void Define( 
				Flowgraph::Node *sym, 
				Flowgraph::Node *value,
				Symbol::Type::Enum kind );
		Flowgraph::Node *Result( Scope &context ) const;
		bool IsMemberizable( Symbol::Type::Enum kind ) const;
		
	protected:	
		Flowgraph::Node *ObjectFunction() const;
		Flowgraph::Node *WrapObject( Flowgraph::Node *exp ) const;
		Flowgraph::Node *StandardSetter() const;
		Flowgraph::Node *CustomSetter( Flowgraph::Node *sym ) const;
		Flowgraph::Node *StandardGetter() const;
		Flowgraph::Node *WrapGetter( Flowgraph::Node *exp ) const;
		
	private:
		Flowgraph::Pool &_pool;
		Flowgraph::Node *_members;
		bool _anyMembersDefined;
};

} // namespace Semantics

# endif //memberdispatch_h
