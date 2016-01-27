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

#ifndef ast_subop_h
#define ast_subop_h

#include "ast/expression.h"
#include "ast/monop.h"

namespace AST {

class SubOp : public Expression
{
	typedef Expression inherited;
	public:
		SubOp(
				const Expression *base,
				const Expression *arg,
				const SourceLocation &loc );
		~SubOp();
		const Expression *Exp() const { return _exp; }
		const Expression *Sub() const { return _sub; }
		virtual void CollectSyncs( std::queue<const class Sync*> *list ) const;
	private:
		const Expression *_exp;
		const Expression *_sub;
};

class Call : public SubOp
{
	public:
		Call(
				const Expression *exp,
				const Arguments *arg,
				const SourceLocation &loc ) :
			SubOp( exp, arg, loc ) { assert( arg && arg->IsArguments() ); }
		const Arguments *Arg() const { return Sub()->AsArguments(); }
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		virtual bool IsACall() const { return true; }
		std::string ToString() const
				{ return Exp()->ToString() + "(" + Arg()->ToString() + ")"; }
};

class Invoke : public SubOp
{
	public:
		Invoke(
				const Expression *object,
				const Expression *arg,
				const SourceLocation &loc ) :
			SubOp( object, arg, loc ) {}
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const;
};

class Lambda : public SubOp
{
	public:
		Lambda(
				const Expression *exp,
				const Expression *param,
				const SourceLocation &loc ) :
			SubOp( exp, param, loc ) {}
		const Expression *Param() const { return Sub(); }
		Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const;
};

class Lookup : public SubOp
{
	public:
		Lookup(
				const Expression *exp,
				const Expression *key,
				const SourceLocation &loc ) :
			SubOp( exp, key, loc ) { assert( key ); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const
				{ return Exp()->ToString() + "[" + Sub()->ToString() + "]"; }
};

class Member : public SubOp
{
	public:
		Member(
				const Expression *exp,
				const Expression *ident,
				const SourceLocation &loc ) :
			SubOp( exp, ident, loc ) { assert( ident ); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		virtual bool IsAMember() const { return true; }
		virtual const Member *AsMember() const { return this; }
		std::string ToString() const
				{ return Exp()->ToString() + "." + Sub()->ToString(); }
};

} // namespace AST

#endif //monop_h
