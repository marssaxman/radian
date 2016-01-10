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

#ifndef LINKER_ASSEMBLER_H
#define LINKER_ASSEMBLER_H

#include <string>
#include <map>
#include <asmjit.h>

typedef asmjit::Label assembler_label;
class assembler: public asmjit::X86Assembler
{
private:
	typedef asmjit::X86Assembler inherited;
	asmjit::JitRuntime runtime;
	std::map<std::string, asmjit::Label> label_names;
public:
	assembler(): inherited(&runtime) {}
	void bind(std::string name) { inherited::bind(label(name)); }
	asmjit::Label label(std::string name);
};

#endif //LINKER_ASSEMBLER_H

