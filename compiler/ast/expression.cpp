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


#include <assert.h>
#include "expression.h"
#include "binop.h"
#include "pool.h"

using namespace AST;

Expression *Expression::Reassociate()
{
	// The parser creates an expression graph without regard to precedence.
	// Each new operator takes the previous expression as its left operand and
	// the next term as its right operand. This method is a utility the parser
	// can use to fix up precedence after it attaches each term: it must call
	// Reassociate on each root node as it is created. The result is the object
	// which should be the actual output node, depending on whether the new
	// operator has a higher or lower precedence than its operand.
	return this;
}

void Expression::UnpackTuple( std::stack<const Expression*> *expList ) const
{
	// If this expression is a tuple, unwind it into a stack, where the top
	// of the stack is the leftmost element. Since this is just an expression,
	// all we have to do is push it on the stack.
	assert( expList );
	expList->push( this );
}
