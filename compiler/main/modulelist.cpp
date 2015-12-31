// Copyright 2009-2011 Mars Saxman.
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
// Radian. If not, see <http://www.gnu.org/licenses/>.

#include "modulelist.h"

using namespace std;

string ModuleRef::UniqueID() const
{
	string out = _target;
	if (_directory.size()) {
		out = _directory + "." + out;
	}
	return out;
}

void ModuleList::ImportModule(
        string target, string directory, const SourceLocation &loc )
{
	_modules.push( ModuleRef( target, directory, loc ) );
}

