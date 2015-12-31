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

#include "flowfuncs.h"

using namespace Flowgraph;

void Self::FormatString( NodeFormatter *formatter ) const
{
	formatter->Element( "self" );
}

Function::Function( Node *exp, unsigned int arity ) :
	Node(),
	_exp(exp),
	_arity(arity),
	_name(DefaultName())
{
}

Function::Function( Node *exp, unsigned int arity, std::string name ) :
	Node(),
	_exp(exp),
	_arity(arity),
	_name(name)
{
}

std::string Function::DefaultName() const
{
	// Synthesize a reasonably unique default name for this function, which
	// we can use in absence of any assigned name annotations.
	return "block_" + numtostr_dec( _arity ) + "_" + UniqueID();
}

std::string Function::ToString() const
{
	NodeFormatter formatter;
	formatter.Begin( "\'" + Name() + "\'" );
	_exp->FormatString( &formatter );
	formatter.End();
	return formatter.Result();
}

void Function::FormatString( NodeFormatter *formatter ) const
{
	// do not include our body. Just print a reference to this function.
	formatter->Element( "\'" + Name() + "\'" );
}

void Parameter::FormatString( NodeFormatter *formatter ) const
{
	formatter->Element( "param_" + numtostr_dec(_index) );
}

void Slot::FormatString( NodeFormatter *formatter ) const
{
	formatter->Element( "slot_" + numtostr_dec(_index) );
}
