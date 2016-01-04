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

#ifndef semantics_closures_function_h
#define semantics_closures_function_h

#include "semantics/closures/closure.h"

namespace Semantics {

class Function : public Closure
{
	typedef Closure inherited;
	public:
		Function( Flowgraph::Pool &pool, Scope *context );
		void Enter( const AST::FunctionDeclaration &it );
		Scope *Exit( const SourceLocation &loc );
		Flowgraph::Node *CaptureLambda( 
				const AST::Expression *param, const AST::Expression *exp );
		std::string FullyQualifiedName() const;

	protected:
		void EnterFunction( 
				const AST::Expression *param, 
				const AST::Expression *exp, 
				const SourceLocation &loc );
		Flowgraph::Node *ExitFunction();
		
	private:
		Token _functionName;
		SourceLocation _beginLoc;

};

} // namespace Semantics

#endif //function_h
