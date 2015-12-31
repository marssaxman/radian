// Copyright 2009-2012 Mars Saxman.
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


#ifndef error_h
#define error_h

#include "sourcelocation.h"
#include <string>
#include <vector>

class Error
{
	public:
		struct Type { enum Enum {
			// Placeholder zero value
			None,

			// Lexical errors
			LoadProgramFileFailed,
			ImportFailed,
			BadToken,
			UnknownToken,
			
			// Syntax errors
			UnknownExprAsStatement,
			UnknownLineEnd,
			UnknownDeclarationValue,
			UnexpectedEOF,
			UnexpectedEOL,
			StatementExpectsIdentifier,
			DeclarationExpectsIdentifier,
			EndExpectsIdentifier,
			UnknownExpressionToken,
			EmptySubexpression,
			EmptyList,
			EmptyMap,
			MissingLeftParen,
			MissingRightParen,
			MissingRightBracket,
			MissingRightBrace,
			UnmatchedBeginBlock,
			UnmatchedEndBlock,
			InsufficientIndentation,
			ExcessiveIndentation,
			ForLoopExpectsInKeyword,
			ForLoopExpectsBlockBegin,
			MutatorInsideExpression,

			// Semantics errors
			NotYetImplemented,
			MemberMustBeIdentifier,
			SingleValueExpected,
			Undefined,
			AssignLhsMustBeIdentifier,
			MutatorNeedsMemberIdentifier,
			AlreadyDefined,
			ParamExpectsIdentifier,
			ElseOperatorWithoutIf,
			IfOperatorWithoutElse,
			ElseStatementOutsideIfBlock,
			ElseStatementAfterFinal,
			YieldInsideMemberDispatch,
			ObjectMemberRedefinition,
			ModuleMemberRedefinition,
			FunctionRedefinition,
			ContextVarRedefinition,
			ConstantRedefinition,
			SelfConstantRedefinition,
			ImportRedefinition,
			MemberRedefinition,
			ImportSourceMustBeIdentifier,
			SubscriptNonFunction,
			MapElementsMustBePairs,
			SyncInsideGenerator,
			YieldInsideAsyncTask,
			DirectMemberReference,

			// Runtime errors
			FalseAssertion,
			VoidInvocation,
			InvalidTypeAssertion,
			MissingMethod,

			// magic constant giving us the number of legal error codes
			COUNT
		}; };
		Error( Type::Enum code, const SourceLocation &loc );
		SourceLocation Location() const { return _location; }
		std::string ToString() const;

	protected:
		Type::Enum _type;
		SourceLocation _location;
};

class Reporter
{
	public:
		virtual ~Reporter() {}
		virtual void Report( const Error &err ) = 0;
		virtual bool HasReceivedReport() const = 0;
		void ReportError( Error::Type::Enum err, const SourceLocation &loc );
};


#endif //error_h
