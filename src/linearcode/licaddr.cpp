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
#include <iostream>
#include "linearcode/licaddr.h"
#include "utility/numtostr.h"

using namespace std;
using namespace LIC;

Addr::Addr() :
	_type(Type::Void),
	_register(0)
{
}

Addr::Addr( unsigned int reg ) :
	_type(Type::Register),
	_register(reg)
{
}

Addr::Addr( std::string data ) :
	_type(Type::Data),
	_data(data),
	_register(0)
{
}

Addr::Addr( const Addr& other ):
	_type(other._type),
	_data(other._data),
	_register(other._register)
{
}

string Addr::ToString() const
{
	switch (_type) {
		case Type::Void: return "nil";
		case Type::Data: return "\"" + _data + "\"";
		case Type::Register: return "R" + numtostr_dec( _register );
		case Type::Link: return _data;
		case Type::Intrinsic: return _data;
		case Type::Index: return "#" + numtostr_dec( _register );
		default: assert( false ); return "";
	}
}

void Addr::Dump() const
{
	cout << ToString() << endl;
}

