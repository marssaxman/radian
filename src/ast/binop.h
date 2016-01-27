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

#ifndef ast_binop_h
#define ast_binop_h

#include "ast/expression.h"
#include "lex/token.h"
#include "lex/sourcelocation.h"

namespace AST {
class Analyzer;

class BinOp : public Expression
{
	public:
		~BinOp();
		static bool IsBinOpToken( const Token &tk );
		static BinOp *Create(
				Expression *left, const Token &op, Expression *right );
		const Expression *Left() const { return _left; }
		const Expression *Right() const { return _right; }
		bool IsABinOp() const { return true; }
		virtual Expression *Reassociate();
		virtual std::string ToString() const;
		virtual void CollectSyncs( std::queue<const class Sync*> *list ) const;
	protected:
		BinOp( Expression *left, Expression *right );
		virtual std::string OpString() const = 0;
	private:
		Expression *_left;
		Expression *_right;
};

class OpIf : public BinOp
{
	public:
		OpIf( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::IfElse; }
		Association::Enum Associativity() const { return Association::Right; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "if"; }
};

class OpElse : public BinOp
{
	public:
		OpElse( Expression *left, Expression *right ) : BinOp( left, right ) {}
		virtual bool IsAOpElse() const { return true; }
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::IfElse; }
		Association::Enum Associativity() const { return Association::Right; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "else"; }
};

class OpTuple : public BinOp
{
	public:
		OpTuple( Expression *left, Expression *right ) : BinOp( left, right ) {}
		bool IsAOpTuple() const { return true; }
		const OpTuple *AsOpTuple() const { return this; }
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Tuple; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		virtual void UnpackTuple(
				std::stack<const Expression*> *explist ) const;
		virtual std::string ToString() const;
	protected:
		std::string OpString() const { return ","; }
};

class OpPair : public BinOp
{
	public:
		OpPair( Expression *left, Expression *right ) : BinOp( left, right ) {}
		bool IsAOpPair() const { return true; }	
		const OpPair *AsOpPair() const { return this; }
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Pair; }
		Association::Enum Associativity() const { return Association::Right; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "=>"; }
};

class OpEQ : public BinOp
{
	public:
		OpEQ( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "="; }
};

class OpNE : public BinOp
{
	public:
		OpNE( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "!="; }
};

class OpGE : public BinOp
{
	public:
		OpGE( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return ">="; }
};

class OpGT : public BinOp
{
	public:
		OpGT( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return ">"; }
};

class OpLE : public BinOp
{
	public:
		OpLE( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "<="; }
};

class OpLT : public BinOp
{
	public:
		OpLT( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "<"; }
};

class OpAdd : public BinOp
{
	public:
		OpAdd( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::AddSubtract; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "+"; }
};

class OpSubtract : public BinOp
{
	public:
		OpSubtract( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::AddSubtract; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "-"; }
};

class OpMultiply : public BinOp
{
	public:
		OpMultiply( 
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::MultiplyDivide; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "*"; }
};

class OpDivide : public BinOp
{
	public:
		OpDivide(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::MultiplyDivide; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "/"; }
};

class OpModulus : public BinOp
{
	public:
		OpModulus(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::MultiplyDivide; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "mod"; }
};

class OpExponent : public BinOp
{
	public:
		OpExponent(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Exponent; }
		Association::Enum Associativity() const { return Association::Right; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "**"; }
};

class OpAnd : public BinOp
{
	public:
		OpAnd( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Logic; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "and"; }
};

class OpOr : public BinOp
{
	public:
		OpOr( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Logic; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "or"; }
};

class OpXor : public BinOp
{
	public:
		OpXor( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Logic; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "xor"; }
};

class OpHas : public BinOp
{
	public:
		OpHas( Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Compare; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "has"; }
};

class OpAssert : public BinOp
{
	public:
		OpAssert(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Assertion; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "as"; }
};

class OpConcat : public BinOp
{
	public:
		OpConcat(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::AddSubtract; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "&"; }
};

class OpShiftLeft : public BinOp
{
	public:
		OpShiftLeft(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Bitwise; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return "<<"; }
};

class OpShiftRight : public BinOp
{
	public:
		OpShiftRight(
				Expression *left, Expression *right ) : BinOp( left, right ) {}
		PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Bitwise; }
		Association::Enum Associativity() const { return Association::Left; }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
	protected:
		std::string OpString() const { return ">>"; }
};


} // namespace AST

#endif	// binop_h
