// Copyright 2016 Mars Saxman.
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

#include "assembler.h"

asmjit::Label assembler::label(std::string name)
{
	auto iter = label_names.find(name);
	if (iter != label_names.end()) {
		return iter->second;
	}
	asmjit::Label l = inherited::newLabel();
	label_names[name] = l;
	return l;
}

