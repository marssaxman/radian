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

#ifndef semantics_roots_program_h
#define semantics_roots_program_h

#include "semantics/roots/root.h"

namespace Semantics {

class ProgramRoot : public Root
{
	public:
		ProgramRoot( Flowgraph::Pool &pool, Reporter &log );
		std::string FullyQualifiedName() const { return ""; }
	protected:
		virtual Flowgraph::Node *ExitRoot();
};

} // namespace Semantics

#endif //program_h
