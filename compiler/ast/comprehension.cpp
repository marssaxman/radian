// Copyright 2010-2012 Mars Saxman.
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


#include "comprehension.h"

using namespace AST;

ListComprehension::ListComprehension( 
	const Expression *output, 
	const Expression *variable, 
	const Expression *input, 
	const Expression *predicate,
	const SourceLocation &loc ):
	Expression(loc),
	_output(output),
	_variable(variable),
	_input(input),
	_predicate(predicate)
{
	// Output and predicate functions are optional, but the input sequence and
	// induction variable are not.
	assert( _variable );
	assert( _input );
}

ListComprehension::~ListComprehension()
{
	if (_output) delete _output;
	delete _variable;
	delete _input;
	if (_predicate) delete _predicate;
}

std::string ListComprehension::ToString() const
{
	std::string out = "each ";
	if (_output) out += _output->ToString() + " from";
	out += _variable->ToString() + " in " + _input->ToString();
	if (_predicate) out += " where " + _predicate->ToString();
	return out;
}

void ListComprehension::CollectSyncs( std::queue<const Sync*> *list ) const
{
	// Left to right depth first search for Sync operators.
	if (_output) _output->CollectSyncs( list );
	if (_variable) _variable->CollectSyncs( list );
	if (_input) _input->CollectSyncs( list );
	if (_predicate) _predicate->CollectSyncs( list );
}
