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

#include "dfg.h"

namespace dfg {

void error::accept(visitor &v) const
{
	v.visit(*this);
}

void literal_int::accept(visitor &v) const
{
	v.visit(*this);
}

void param_val::accept(visitor &v) const
{
	v.visit(*this);
}

void block_ref::accept(visitor &v) const
{
	v.visit(*this);
}

void unary::accept(visitor &v) const
{
	_source.accept(v);
	v.visit(*this);
}

void field::accept(visitor &v) const
{
	_source.accept(v);
	v.visit(*this);
}

void binary::accept(visitor &v) const
{
	_left.accept(v);
	_right.accept(v);
	v.visit(*this);
}

void select::accept(visitor &v) const
{
	_cond.accept(v);
	_then_val.accept(v);
	_else_val.accept(v);
	v.visit(*this);
}

void variadic::accept(visitor &v) const
{
	for (auto &i: _sources) {
		i.get().accept(v);
	}
	v.visit(*this);
}

builder::builder():
	_param(*new param_val())
{
	_code.emplace_back(&_param);
}


} // namespace dfg







