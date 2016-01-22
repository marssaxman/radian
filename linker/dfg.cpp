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
	v.enter(*this);
	source.accept(v);
	v.leave(*this);
}

void field::accept(visitor &v) const
{
	v.enter(*this);
	source.accept(v);
	v.leave(*this);
}

void binary::accept(visitor &v) const
{
	v.enter(*this);
	left.accept(v);
	right.accept(v);
	v.leave(*this);
}

void select::accept(visitor &v) const
{
	v.enter(*this);
	cond.accept(v);
	thenval.accept(v);
	elseval.accept(v);
	v.leave(*this);
}

void variadic::accept(visitor &v) const
{
	v.enter(*this);
	for (auto &i: sources) {
		i.get().accept(v);
	}
	v.leave(*this);
}

block::block(std::string n):
	name(n)
{
	code.emplace_back(new param_val);
}

std::string block::type() const
{
	// The link ID includes the type signature as a dot extension.
	size_t pos = name.find_last_not_of('.');
	if (pos != std::string::npos) {
		return name.substr(pos + 1);
	} else {
		return std::string();
	}
}

void block::accept(visitor &v) const
{
	v.enter(*this);
	code.back()->accept(v);
	v.leave(*this);
}

block &unit::make(std::string name)
{
	block *out = new block(name);
	blocks.emplace_back(out);
	return *out;
}

void unit::accept(visitor &v) const
{
	for (auto &b: blocks) {
		b->accept(v);
	}
}

} // namespace dfg







