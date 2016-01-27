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

#include "ast/subop.h"

using namespace AST;

SubOp::SubOp(
		const Expression *exp,
		const Expression *sub,
		const SourceLocation &loc ) :
	Expression( loc ),
	_exp(exp),
	_sub(sub)
{
	assert( exp );
}

SubOp::~SubOp()
{
	delete _exp;
	if (_sub) delete _sub;
}

void SubOp::CollectSyncs( std::queue<const class Sync*> *list ) const
{
	// Left to right depth first search.
	// we don't technically have a fixed left to right on a subop, but for the
	// most part the base comes first, then the sub. The only exception is the
	// lambda capture operation, where the sub is a parameter list, but we'll
	// never have a (legal) sync in the parameter list there, so we can ignore
	// it and pretend the sub always follows the base.
	_exp->CollectSyncs( list );
	if (_sub) _sub->CollectSyncs( list );
}

std::string Invoke::ToString() const
{
	std::string out = "invoke(";
	out += Exp()->ToString();
	if (Sub()) {
		out += ": ";
		out += Sub()->ToString();
	}
	out += ")";
	return out;
}

std::string Lambda::ToString() const
{
	std::string out = "capture(";
	if (Param()) {
		out += Param()->ToString();
		out += ": ";
	}
	out += Exp()->ToString();
	out += ")";
	return out;
}

