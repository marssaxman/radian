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


#include "value.h"

using namespace Flowgraph;

Value::Value( Type::Enum code, std::string value ):
    Node(),
    _value(value),
    _type(code)
{
}

void Value::FormatString( NodeFormatter *formatter ) const
{
	switch (_type) {
		case Value::Type::Void: {
			// Do nothing. Void value just ends a list.
		} break;
		case Value::Type::Symbol: {
			formatter->Element( ":" + _value );
		} break;
		case Value::Type::String: {
			// This is wrong. Should escape any inner quote characters.
			formatter->Element( "\"" + _value + "\"" );
		} break;
		default: {
			formatter->Element( Contents() );
		}
	}
}
