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


#ifndef expression_h
#define expression_h

#include <stack>
#include "sourcelocation.h"
#include "token.h"
#include "node.h"

namespace AST {

class Expression
{
	public:
		struct PrecedenceLevel { enum Enum {
			Bogus = 0,
			Tuple,
			Pair,

			Assertion,
			IfElse,
			Logic,
			Compare,
			Bitwise,
			AddSubtract,
			MultiplyDivide,
			Exponent,
			Primary
		}; };

		struct Association { enum Enum {
			Left,
			Right,
			None
		}; };

		Expression( const SourceLocation &loc ) : _location(loc) {}
		virtual ~Expression() {}
		virtual Expression *Reassociate();
		SourceLocation Location() const { return _location; }

		// Double dispatcher: call the expression analyzer with this, using our
		// static type to pick the correct overload.
		virtual Flowgraph::Node *SemGen(
				class ExpressionAnalyzer *it ) const = 0;
		// Depth-first, left-to-right traversal of the tree looking for sync
		// expressions, which must be evaluated before all other expressions.
		virtual void CollectSyncs(
				std::queue<const class Sync*> *list ) const = 0;

		virtual bool IsABinOp() const { return false; }
		virtual bool IsAnIdentifier() const { return false; }
		virtual const class Identifier *AsIdentifier() const
				{ assert( false ); return NULL; }
		virtual bool IsACall() const { return false; }
		virtual bool IsAOpTuple() const { return false; }
		virtual const class OpTuple *AsOpTuple() const
				{ assert( false ); return NULL; }
		virtual bool IsAOpElse() const { return false; }
		virtual bool IsAOpPair() const { return false; }
		virtual const class OpPair *AsOpPair() const
				{ assert( false ); return NULL; }
		virtual bool IsAMember() const { return false; }
		virtual const class Member *AsMember() const
				{ assert( false ); return NULL; }
		virtual bool IsArguments() const { return false; }
		virtual const class Arguments *AsArguments() const
				{ assert( false ); return NULL; }
		virtual bool IsAList() const { return false; }
		virtual const class List *AsList() const
				{ assert( false ); return NULL; }

		virtual void UnpackTuple(
				std::stack<const Expression*> *expList ) const;

		virtual PrecedenceLevel::Enum Precedence() const
				{ return PrecedenceLevel::Primary; }
		virtual Association::Enum Associativity() const
				{ return Association::None; }
		virtual std::string ToString() const = 0;
	private:
		SourceLocation _location;
};

class ExpressionAnalyzer
{
	public:
		virtual ~ExpressionAnalyzer() {}
		// We should have one SemGen method for each expression node class
		// Each one will process the given expression node and return some
		// flowgraph node representation.
		virtual Flowgraph::Node *SemGen( const class Arguments &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class BooleanLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Call &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Dummy &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class FloatLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Identifier &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class IntegerLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Invoke &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Lambda &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class List &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class ListComprehension &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Lookup &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Map &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Member &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class NegationOp &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class NotOp &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpIf &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpElse &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpTuple &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpPair &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpEQ &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpNE &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpGE &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpGT &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpLE &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpLT &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpAdd &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpSubtract &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpMultiply &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpDivide &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpModulus &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpExponent &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpAnd &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpOr &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpXor &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpHas &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpAssert &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpConcat &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpShiftLeft &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class OpShiftRight &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class RealLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class StringLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class SubExpression &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class SymbolLiteral &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Sync &it ) = 0;
		virtual Flowgraph::Node *SemGen( const class Throw &it ) = 0;
};

} // namespace AST

#endif // expression_h
