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


#ifndef root_h
#define root_h

#include "scope.h"

namespace Semantics {

// Root is the ultimate context, the outermost of all scopes.
class Root : public Scope
{
	public:
		Root( Flowgraph::Pool &pool, Reporter &log );
		~Root();
		void Report( const Error &err );
		bool HasReceivedReport() const {  return _errors.HasReceivedReport(); }
		Scope *Exit( const SourceLocation &loc );

	protected:
		Symbol CaptureFromContext( Flowgraph::Node *name );
		void RebindInContext(
				Flowgraph::Node *name,
				Flowgraph::Node *value,
				const SourceLocation &loc );
		virtual Flowgraph::Node *ExitRoot(void) = 0;

	private:
		Reporter &_errors;
};

} // namespace Semantics

#endif //root_h
