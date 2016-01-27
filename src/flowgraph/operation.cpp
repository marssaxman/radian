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

#include <assert.h>
#include <algorithm>
#include "flowgraph/operation.h"
#include "flowgraph/value.h"

using namespace Flowgraph;

Operation::Operation( Type::Enum which, Node *left, Node *right ):
	Node(),
	_type(which),
	_left(left),
	_right(right),
	_isInductionVar(false),
	_minParameterCount(0)
{
	assert( left );
	assert( right );
	unsigned leftCount = left->MinParameterCount();
	unsigned rightCount = right->MinParameterCount();
	if (left->IsInductionVar()) {
		_isInductionVar = right->IsInductionVar() || (0 == rightCount);
	}
	else if (right->IsInductionVar()) {
		_isInductionVar = (0 == leftCount);
	}
	_minParameterCount = std::max(leftCount, rightCount);
}

void Operation::FormatString( NodeFormatter *formatter ) const
{
	std::string opname;
	switch (_type) {
		case Type::Call: opname = "call"; break;
		case Type::Capture: opname = "lambda"; break;
		case Type::Arg: opname = ""; break;
		case Type::Loop: opname = "loop"; break;
		default: assert(false);
	}
	if (!opname.empty()) {
		formatter->Begin( opname );
	}
	if (!FormatMethodCallPattern( formatter )) {
		_left->FormatString( formatter );
	}
	_right->FormatString( formatter );
	if (!opname.empty()) {
		formatter->End();
	}
}

bool Operation::FormatMethodCallPattern( NodeFormatter *formatter ) const
{
	// Look for a specific pattern where we look up a method from some object
	// and then invoke it. We'll print out a simpler, less wordy string that
	// makes the meaning clearer.
	// A method call is a call
	if (_type != Type::Call) return false;
	// With at least one argument
	if (!_right->IsAnArg()) return false;
	// Go figure out what the argument's value is
	Operation *walker = _right->AsOperation();
	Node *arg = walker->Right();
	while (walker->Left()->IsAnArg()) {
		walker = walker->Left()->AsOperation();
		arg = walker->Right();
	}
	// The call's target must be another call
	if (!_left->IsAnOperation()) return false;
	Operation *target = _left->AsOperation();
	if (target->Type() != Operation::Type::Call) return false;
	// And it must have exactly one argument
	if (!target->Right()->IsAnArg()) return false;
	walker = target->Right()->AsOperation();
	// The argument must be a symbol, which is the method name
	if (!walker->Right()->IsASymbol()) return false;
	Value *sym = walker->Right()->AsValue();
	// And its target must equal this call's first argument
	if (target->Left() != arg) return false;
	// yay, we match the pattern
	formatter->Element( "method<" + sym->AsValue()->Contents() + ">" );
	return true;
}
