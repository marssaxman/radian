// Copyright 2009-2016 Mars Saxman.
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

#ifndef semantics_closures_objects_h
#define semantics_closures_objects_h

#include "semantics/closures/closure.h"
#include "semantics/memberdispatch.h"

namespace Semantics {

class Object : public Closure
{
	typedef Closure inherited;
	public:
		Object( Flowgraph::Pool &pool, Scope *context );
		void Enter( const AST::ObjectDeclaration &it );
		Scope *Exit( const SourceLocation &loc );
		bool NeedsImplicitSelf() const { return true; };
		Flowgraph::Node *ImplicitSelfName() const { return _pool.Sym_Self(); }
		std::string FullyQualifiedName() const;

	protected:
		void Define(
				Flowgraph::Node *sym,
				Flowgraph::Node *value,
				Symbol::Type::Enum kind,
				const SourceLocation &loc );
		void Assign(
				Flowgraph::Node *sym,
				Flowgraph::Node *value,
				const SourceLocation &loc );

	private:
		bool _declarationsBecomeMembers;
		MemberDispatch _members;
		Token _objectName;
};

class Method : public Closure
{
	typedef Closure inherited;
	public:
		Method( Flowgraph::Pool &pool, Scope *context );
		void Enter( const AST::MethodDeclaration &it );
		Scope *Exit( const SourceLocation &loc );
		std::string FullyQualifiedName() const;
	
	protected:
		Symbol::Type::Enum SelfParameterType() const
				{ return Symbol::Type::Var; }
	
	private:
		Token _methodName;
};

} // namespace Semantics

#endif //objects_h
