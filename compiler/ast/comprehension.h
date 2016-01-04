// Copyright 2010-2016 Mars Saxman.
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

#ifndef ast_comprehension_h
#define ast_comprehension_h

#include "ast/expression.h"

namespace AST {

class ListComprehension : public Expression
{
	public:
		ListComprehension( 
			const Expression *output, 
			const Expression *variable, 
			const Expression *input, 
			const Expression *predicate,
			const SourceLocation &loc );
		~ListComprehension();
		const Expression *Output() const { return _output; }
		const Expression *Variable() const { return _variable; }
		const Expression *Input() const { return _input; }
		const Expression *Predicate() const { return _predicate; }
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const;
		void CollectSyncs( std::queue<const class Sync*> *list ) const;
	protected:
		const Expression *_output;
		const Expression *_variable;
		const Expression *_input;
		const Expression *_predicate;
};

}	// namespace AST

#endif // comprehension_h

