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
// Radian.  If not, see <http://www.gnu.org/licenses/>.


#include "monop.h"

using namespace AST;

MonOp::MonOp( const Expression *exp, const SourceLocation &loc ):
	Expression(loc),
	_exp(exp)
{
}

void MonOp::CollectSyncs( std::queue<const Sync*> *list ) const
{
	// Dig deeper: collect syncs from the expression we are wrapping.
	if (_exp) _exp->CollectSyncs( list );
}

Arguments::Arguments( const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
}

List::List( const Expression* items, const SourceLocation &loc ):
	MonOp(items, loc)
{
	assert( items );
}

Map::Map( const Expression *items, const SourceLocation &loc ):
	MonOp(items, loc)
{
	assert( items );
}

NegationOp::NegationOp( const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
	assert( exp );
}

NotOp::NotOp( const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
	assert( exp );
}

SubExpression::SubExpression(
		const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
	assert( exp );
}

Sync::Sync( const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
}

std::string Sync::ToString() const
{
	std::string out = "sync";
	if (Exp()) {
		out = out + "(" + Exp()->ToString() + ")";
	}
	return out;
}

void Sync::CollectSyncs( std::queue<const Sync*> *list ) const
{
	// This is a depth first search, so dig first, then add self to the queue.
	inherited::CollectSyncs( list );
	assert( list );
	list->push( this );
}

Throw::Throw( const Expression *exp, const SourceLocation &loc ) :
	MonOp(exp, loc)
{
	assert( exp );
}


