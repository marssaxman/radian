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


#ifndef monop_h
#define monop_h

#include "expression.h"

namespace AST {

class MonOp : public Expression
{
	typedef Expression inherited;
	public:
		MonOp( const Expression *exp, const SourceLocation &loc );
		~MonOp() { if (_exp) delete _exp; }
		const Expression *Exp() const { return _exp; }
		virtual void CollectSyncs( std::queue<const class Sync*> *list ) const;
	private:
		const Expression *_exp;
};

class Arguments : public MonOp
{
	typedef MonOp inherited;
	public:
		Arguments( const Expression *tuple, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		bool IsArguments() const { return true; }
		const Arguments *AsArguments() const { return this; }
		std::string ToString() const { return Exp() ? Exp()->ToString() : ""; }
};

class List : public MonOp
{
	typedef MonOp inherited;
	public:
		List( const Expression* items, const SourceLocation &loc );
		const Expression *Items() const { return Exp(); }
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		bool IsAList(void) const { return true; }
		const List *AsList(void) const { return this; }
		std::string ToString() const { return "[" + Exp()->ToString() + "]"; }
};

class Map : public MonOp
{
	typedef MonOp inherited;
	public:
		Map( const Expression *items, const SourceLocation &loc );
		const Expression *Items() const { return Exp(); }
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "{" + Exp()->ToString() + "}"; }
};

class NegationOp : public MonOp
{
	typedef MonOp inherited;
	public:
		NegationOp( const Expression *exp, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "-" + Exp()->ToString(); }
};

class NotOp : public MonOp
{
	typedef MonOp inherited;
	public:
		NotOp( const Expression *exp, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "not " + Exp()->ToString(); }
};

class SubExpression : public MonOp
{
	typedef MonOp inherited;
	public:
		SubExpression( const Expression *exp, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "(" + Exp()->ToString() + ")"; }
};

class Sync : public MonOp
{
	typedef MonOp inherited;
	public:
		Sync( const Expression *exp, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const;
		virtual void CollectSyncs( std::queue<const class Sync*> *list ) const;
};

class Throw : public MonOp
{
	typedef MonOp inherited;
	public:
		Throw( const Expression *exp, const SourceLocation &loc );
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const
				{ return "throw(" + Exp()->ToString() + ")"; }
};

} // namespace AST

#endif //monop_h
