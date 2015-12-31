// Copyright 2009-2013 Mars Saxman.
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

#include "intrinsics.h"
#include <stdio.h>

using namespace Flowgraph;

std::string Intrinsic::Link() const
{
	switch (_ID) {
		case ID::IsNotVoid: return "is_not_void";
		case ID::Catch: return "catch_exception";
		case ID::Throw: return "throw_exception";
		case ID::IsNotExceptional: return "is_not_exceptional";
		case ID::Parallelize: return "parallelize";
		case ID::Tuple: return "make_tuple";
		case ID::Map_Blank: return "map_blank";
		case ID::List: return "list";
		case ID::List_Blank: return "list_empty";	// named oddly in C code
		case ID::Loop_Sequencer: return "loop_sequencer";
		case ID::Loop_Task: return "loop_task";
		case ID::Char_From_Int: return "char_from_int";
		case ID::FFI_Load_External: return "FFI_Load_External";
		case ID::FFI_Describe_Function: return "FFI_Describe_Function";
		case ID::FFI_Call: return "FFI_Call";
		case ID::Read_File: return "Read_File";
		case ID::Write_File: return "Write_File";
		case ID::Debug_Trace: return "debug_trace";
        case ID::Sin: return "math_sin";
        case ID::Cos: return "math_cos";
        case ID::Tan: return "math_tan";
        case ID::Asin: return "math_asin";
        case ID::Acos: return "math_acos";
        case ID::Atan: return "math_atan";
        case ID::Atan2: return "math_atan2";
        case ID::Sinh: return "math_sinh";
        case ID::Cosh: return "math_cosh";
        case ID::Tanh: return "math_tanh";
        case ID::Asinh: return "math_asinh";
        case ID::Acosh: return "math_acosh";
        case ID::Atanh: return "math_atanh";
        case ID::To_Float: return "to_float";
		case ID::Floor_Float: return "floor_float";
		case ID::Ceiling_Float: return "ceiling_float";
		case ID::Truncate_Float: return "truncate_float";
		default:
			fprintf(stderr, "unknown intrinsic link ID %d\n", _ID);
			assert(false);
	}
}

void Intrinsic::FormatString( NodeFormatter *formatter ) const
{
	if (_ID < ID::COUNT) {
		formatter->Element( Link() );
	} else {
		formatter->Element( "intrinsic_" + numtostr_dec(_ID) );
	}
}
