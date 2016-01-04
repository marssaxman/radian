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

#include "ast/terminal.h"
#include <cstdlib>

using namespace AST;

Identifier::Identifier( std::string tk, const SourceLocation &loc ) :
    Expression( loc ),
    _identifier( tk )
{
}

std::string HexLiteral::Value() const
{
	// Convert the hex value to an integer
	return numtostr_dec( strtoul( _token.Value().c_str(), NULL, 16 ) );
}

std::string OctLiteral::Value() const
{
	// Convert the oct value to an integer
	return numtostr_dec( strtoul( _token.Value().c_str(), NULL, 8 ) );
}

std::string BinLiteral::Value() const
{
	// Convert the bin value to an integer
	return numtostr_dec( strtoul( _token.Value().c_str(), NULL, 2 ) );
}
