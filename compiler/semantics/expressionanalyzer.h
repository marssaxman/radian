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

#ifndef semantics_expressionanalyzer_h
#define semantics_expressionanalyzer_h

#include "error.h"
#include "flowgraph/flowgraph.h"
#include "ast/ast.h"
#include "semantics/symbols.h"
#include "semantics/scope.h"

namespace Semantics {

// Core expression analyzer: turns an expression AST into a data flowgraph.
class ExpressionAnalyzer : protected AST::ExpressionAnalyzer
{
	public:
		ExpressionAnalyzer( Flowgraph::Pool &pool, Scope &context );
		void ProcessSyncs( const AST::Expression *it );
		Flowgraph::Node *Eval( const AST::Expression *it );

	protected:
		Flowgraph::Node *SemGen( const AST::Arguments &it );
		Flowgraph::Node *SemGen( const AST::BooleanLiteral &it );
		Flowgraph::Node *SemGen( const AST::Call &it );
		Flowgraph::Node *SemGen( const AST::Dummy &it );
		Flowgraph::Node *SemGen( const AST::FloatLiteral &it );
		Flowgraph::Node *SemGen( const AST::Identifier &it );
		Flowgraph::Node *SemGen( const AST::IntegerLiteral &it );
		Flowgraph::Node *SemGen( const AST::Invoke &it );
		Flowgraph::Node *SemGen( const AST::Lambda &it );
		Flowgraph::Node *SemGen( const AST::List &it );
		Flowgraph::Node *SemGen( const AST::ListComprehension &it );
		Flowgraph::Node *SemGen( const AST::Lookup &it );
		Flowgraph::Node *SemGen( const AST::Map &it );
		Flowgraph::Node *SemGen( const AST::Member &it );
		Flowgraph::Node *SemGen( const AST::NegationOp &it );
		Flowgraph::Node *SemGen( const AST::NotOp &it );
		Flowgraph::Node *SemGen( const AST::OpIf &it );
		Flowgraph::Node *SemGen( const AST::OpElse &it );
		Flowgraph::Node *SemGen( const AST::OpTuple &it );
		Flowgraph::Node *SemGen( const AST::OpPair &it );
		Flowgraph::Node *SemGen( const AST::OpEQ &it );
		Flowgraph::Node *SemGen( const AST::OpNE &it );
		Flowgraph::Node *SemGen( const AST::OpGE &it );
		Flowgraph::Node *SemGen( const AST::OpGT &it );
		Flowgraph::Node *SemGen( const AST::OpLE &it );
		Flowgraph::Node *SemGen( const AST::OpLT &it );
		Flowgraph::Node *SemGen( const AST::OpAdd &it );
		Flowgraph::Node *SemGen( const AST::OpSubtract &it );
		Flowgraph::Node *SemGen( const AST::OpMultiply &it );
		Flowgraph::Node *SemGen( const AST::OpDivide &it );
		Flowgraph::Node *SemGen( const AST::OpModulus &it );
		Flowgraph::Node *SemGen( const AST::OpExponent &it );
		Flowgraph::Node *SemGen( const AST::OpAnd &it );
		Flowgraph::Node *SemGen( const AST::OpOr &it );
		Flowgraph::Node *SemGen( const AST::OpXor &it );
		Flowgraph::Node *SemGen( const AST::OpHas &it );
		Flowgraph::Node *SemGen( const AST::OpAssert &it );
		Flowgraph::Node *SemGen( const AST::OpConcat &it );
		Flowgraph::Node *SemGen( const AST::OpShiftLeft &it );
		Flowgraph::Node *SemGen( const AST::OpShiftRight &it );
		Flowgraph::Node *SemGen( const AST::RealLiteral &it );
		Flowgraph::Node *SemGen( const AST::StringLiteral &it );
		Flowgraph::Node *SemGen( const AST::SubExpression &it );
		Flowgraph::Node *SemGen( const AST::SymbolLiteral &it );
		Flowgraph::Node *SemGen( const AST::Sync &it );
		Flowgraph::Node *SemGen( const AST::Throw &it );

	private:
		Scope &_context;
		Flowgraph::Pool &_pool;

		// We'll keep track of the last sync we visited, for safety checks.
		const AST::Sync *_lastProcessedSync;
		// Thunks let us defer or skip subexpression evaluation.
		Flowgraph::Node *GenerateThunk( const AST::Expression *exp );
		// Generate code for an argument list, starting with some prefix.
		Flowgraph::Node *GenArgs(
				const AST::Arguments &it, Flowgraph::Node *prefix );
};

} // namespace Semantics

#endif // expressionanalyzer_h
