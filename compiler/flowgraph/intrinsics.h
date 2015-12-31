// Copyright 2010-2013 Mars Saxman.
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

#ifndef intrinsics_h
#define intrinsics_h

#include <string>
#include "node.h"
#include "numtostr.h"

namespace Flowgraph {

class Intrinsic : public Node
{
	friend class Pool;
	public:
		struct ID { enum Enum {
			IsNotVoid,
			Catch,
			Throw,
			IsNotExceptional,
			Parallelize,
			Tuple,
			Map_Blank,
			List,
			List_Blank,
			Loop_Sequencer,
			Loop_Task,
			Char_From_Int,
			FFI_Load_External,
			FFI_Describe_Function,
			FFI_Call,
			Read_File,
			Write_File,
			Debug_Trace,

			Sin,
			Cos,
			Tan,
			Asin,
			Acos,
			Atan,
			Atan2,
			Sinh,
			Cosh,
			Tanh,
			Asinh,
			Acosh,
			Atanh,
			To_Float,
			Floor_Float,
			Ceiling_Float,
			Truncate_Float,

			COUNT
		}; };
		bool IsIntrinsic() const { return true; }
		Intrinsic *AsIntrinsic() { return this; }
		virtual bool IsContextIndependent() const { return true; }
		ID::Enum ID() const { return _ID; }
		std::string Link() const;
	protected:
		Intrinsic( unsigned index ) : Node(), _ID((ID::Enum)index) {}
		void FormatString( NodeFormatter *formatter ) const;
	private:
		ID::Enum _ID;
};

}	// namespace Flowgraph

#endif	//intrinsics_h
