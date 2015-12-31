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


#ifndef module_h
#define module_h

#include "root.h"
#include "memberdispatch.h"

namespace Semantics {

class ModuleRoot : public Root
{
	typedef Root inherited;
	public:
		ModuleRoot( Flowgraph::Pool &pool, Reporter &log, std::string name );
		void EnableBuiltins(void);
		Flowgraph::Node *ExitRoot();
		bool NeedsImplicitSelf() const { return true; };
		Flowgraph::Node *ImplicitSelfName() const { return _selfSym; }
		virtual std::string FullyQualifiedName() const { return _name; }

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
        void BuiltinFunction(
                std::string name, Flowgraph::Intrinsic::ID::Enum id );
        void BuiltinDef( std::string name, Flowgraph::Intrinsic::ID::Enum id );
		void Builtin(
				std::string name,
				Flowgraph::Intrinsic::ID::Enum id,
				Symbol::Type::Enum type );
		MemberDispatch _members;
		std::string _name;
		Flowgraph::Node *_selfSym;
};

} // namespace Semantics

#endif //module_h
