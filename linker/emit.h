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

#ifndef LINKER_EMIT_H
#define LINKER_EMIT_H

#include <vector>
#include <map>
#include <memory>
#include "assembler.h"

class emit
{
public:
	struct value
	{
		enum code
		{
			// intrinsics
			id_param,
			id_env,
			// literal value
			id_lit,
			// arithmetic
			op_add, // l + r
			op_sub, // l - r
			op_mul, // l * r
			op_div, // l / r
			op_quo, // l \ r
			op_rem, // l % r
			// bitwise
			op_shl, // l << r
			op_shr, // l >> r
			op_not, // ~a
			op_and, // l & r
			op_or,  // l | r
			op_xor, // l ^ r
			// comparison
			op_cmp, // l == r
			op_ord, // l <= r
			op_seq, // l <  r
			// branch
			op_sel, // a? b: c
			// structure
			op_pack, // value* => structure
			op_field, // index, structure => value
			// closure
			op_bind, // block, value* => closure
			// array
			op_join, // value* => list with that many items
			op_concat, // list* => list from contents of lists
			op_item, // index, array => value
			op_slice, // start, array, count => array
			op_count, // array => number of items
		} id;
		std::string type;
	protected:
		value(code _id, std::string _type): id(_id), type(_type) {}
	};

	class param: public value
	{
	public:
		param(std::string _type): value(id_param, _type) {}
	};

	class env: public value
	{
	public:
		env(std::string _type): value(id_env, _type) {}
	};

	struct block
	{
		std::string in_type;
		std::string env_type;
		size_t out_exp;
		size_t next_exp;
	};

	emit(block &src, assembler &dest);
};

#endif //LINKER_EMIT_H

