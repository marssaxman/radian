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
// Radian. If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include "error.h"

Error::Error( Error::Type::Enum type, const SourceLocation &loc ):
	_type(type),
	_location(loc)
{
	assert( type != Error::Type::None );
	assert( (int)type < (int)Error::Type::COUNT );
}

std::string Error::ToString() const
{
	// Return this error information in a human-readable format that is also
	// tool-parseable. This means we must use a standardized format which
	// includes readable text. We will return first the location, then the
	// error code number (for the tools), then some string describing the error
	// condition. Eventually we ought to figure out how to localize the error
	// string table, but that's a problem for another day. Of course we have to
	// make sure we keep this string table lined up with the enum constants
	// defined in error.h, but that shouldn't be a problem for a while yet. 
	const char *messages[Error::Type::COUNT] = {
		// Placeholder zero value
		"", // None
		
		// Lexical errors
		"Program file does not exist or could not be opened.", // LoadProgramFileFailed
		"Import failed because the target file does not exist or could not be opened.", // ImportFailed
		"Malformed input character.", //BadToken
		"Unexpected character.", // UnknownToken

		// Syntax errors
		"This expression cannot be used as a statement.", //UnknownExprAsStatement
		"Expected a line break here.", //UnknownLineEnd
		"This token does not make sense; only a colon or an equals-sign can go here.", //UnknownDeclarationValue
		"Wait, we're not done yet! Did not expect the file to end already.", //UnexpectedEOF
		"The statement is not finished yet, which makes this linebreak hard to explain.", //UnexpectedEOL
		"This is not a legal identifier token.", //StatementExpectsIdentifier
		"The declared name should go here, but this is not a legal identifier token.", //DeclarationExpectsIdentifier
		"Expected a block name here.", // EndExpectsIdentifier
		"This symbol does not stand for any value and does not make sense here.", // UnknownExpressionToken
		"Empty subexpression is meaningless.", // EmptySubexpression
		"Empty list cannot be constructed.", // EmptyList
		"Empty map cannot be constructed.", // EmptyMap
		"Missing left parenthesis.", // MissingLeftParen
		"Missing right parenthesis.", // MissingRightParen
		"Missing right bracket.", // MissingRightBracket
		"Missing right brace.", // MissingRightBrace
		"This block does not have a matching end statement.", // UnmatchedBeginBlock
		"This end statement does not match the current open block.", // UnmatchedEndBlock
		"This statement is not indented enough to match its block.",	// InsufficientIndentation
		"This statement is indented too far for its block.",	// ExcessiveIndentation
		"The for statement expects to find the keyword 'in' here.", //ForLoopExpectsInKeyword
		"The for statement always begins a block, but this statement does not end in a colon.", //ForLoopExpectsBlockBegin
        "Can't modify an object as a side-effect of an expression; try this as a statement instead.", //MutatorInsideEXpression

		// Semantics errors
		"This behavior has not yet been implemented. Remind Mars about this.", //NotYetImplemented
		"This should be the name of an object member, but it is not a legal identifier token.", // MemberMustBeIdentifier
		"The context expects a single value, but this is a series of values. Perhaps it needs a pair of brackets.", // SingleValueExpected
		"This name is not defined.", //Undefined
		"Values can be assigned to variable names, but not to any other kind of expression.", //AssignLhsMustBeIdentifier
		"This function call does not make sense; there should be some mutator method call here.", // MutatorNeedsMemberIdentifier
		"This name has already been defined.", // AlreadyDefined
		"This should define the name of a parameter, but it is some other kind of expression.", //ParamExpectsIdentifier
		"There is no If to match this Else.", //ElseOperatorWithoutIf
		"This If operation does not have a matching Else.", //IfOperatorWithoutElse
		"Else statements only work inside an If block.", // ElseStatementOutsideIfBlock
		"An If operation can only have one Else.", // ElseStatementAfterFinal
		"Cannot yield inside an object constructor or module scope.", // YieldInsideMemberDispatch
		"Objects may contain only definitions; assignments and actions are not allowed.", // ObjectMemberRedefinition
		"Modules may contain only definitions; assignments and actions are not allowed.", // ModuleMemberRedefinition
		"This is a function, not a variable, so its value cannot be changed.", //FunctionRedefinition
		"This variable cannot be changed because it was defined outside the current function.", //ContextVarRedefinition
		"This is a definition, not a variable, so its value cannot be changed.", //ConstantRedefinition
		"The self object is immutable inside this function; it can only be altered inside a method.", //SelfConstantRedefinition
		"This is an import, not a variable, so its value cannot be changed.", //ImportRedefinition
		"This object member has already been defined and cannot be changed.", //MemberRedefinition
		"The source directory for this import must be an identifier.", //ImportSourceMustBeIdentifier
		"This is not a function, so it should not have an argument subscript.", //SubscriptNonFunction,
		"Map elements must be key => value pairs.",	//MapElementsMustBePairs
		"Cannot sync inside a function which has already yielded.", //SyncInsideGenerator
		"Cannot yield inside a function which has already synced.",	// YieldInsideAsyncTask
		"Refer to this object member through \"self\" instead of using its name alone.", // DirectMemberReference

		// Consistency errors
		"This condition is not true.", //FalseAssertion
		"Value is void and cannot be invoked.", //VoidInvocation
		"This value does not have the specified type.", // InvalidTypeAssertion
		"The object does not implement the requested method.", //MissingMethod
		};
	assert( _type >= Error::Type::None && _type < Error::Type::COUNT );
	return _location.ToString() + ": " + std::string(messages[_type]);
}

void Reporter::ReportError( Error::Type::Enum err, const SourceLocation &loc )
{
	Report( Error( err, loc ) );
}


