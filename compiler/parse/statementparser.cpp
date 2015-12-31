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


#include "statementparser.h"

using namespace Parser;

Statement::Statement( Iterator<Token> &input, Reporter &log ):
	Core( input, log ),
	_currentStatement(NULL)
{
	if (Next()) {
		if (!IsCurrent( Token::Type::Error )) {
			_currentStatement = ParseStatement();
		} else {
			SyntaxError( Error::Type::BadToken );
		}
		if (!_currentStatement) {
			_currentStatement = new AST::BlankLine( _indentLevel, Location() );
		}
		// If the scanner is not done, its current token must be an EOL.
		// Otherwise there is some unidentified crap at the end of the line -
		// possibly a mismatched bracket or a missing comma.
		Synchronize( Token::Type::EOL, Error::Type::UnknownLineEnd );
	}
}

Token Statement::ExpectDeclIdent()
{
	Token tk = Current();
	Expect(
			Token::Type::Identifier,
			Error::Type::DeclarationExpectsIdentifier );
	return tk;
}

// Statement::ParseStatement
//
// There is more input. Figure out what kind of statement it is and parse it,
// returning a new statement object. We will incidentally update the current
// location value.
//
AST::Statement *Statement::ParseStatement()
{
	// A statement may begin with any number of leading indents.
	_indentLevel = 0;
	while (Match(Token::Type::Indent)) _indentLevel++;

	// The first token following the indent identifies the statement type.
	switch (Current().TokenType()) {
		case Token::Type::KeyAssert: return ParseAssertion();
		case Token::Type::KeyDebug_Trace: return ParseDebugTrace();
		case Token::Type::KeyDef: return ParseDefinition();
		case Token::Type::KeyElse: return ParseElse();
		case Token::Type::KeyEnd: return ParseBlockEnd();
		case Token::Type::KeyFor: return ParseForLoop();
		case Token::Type::KeyFunction: return ParseFunctionDeclaration();
		case Token::Type::KeyIf: return ParseIfThen();
		case Token::Type::KeyImport: return ParseImport();
		case Token::Type::KeyMethod: return ParseMethodDeclaration();
		case Token::Type::KeyObject: return ParseObjectDeclaration();
		case Token::Type::KeySync: return ParseSync();
		case Token::Type::KeyVar: return ParseVarDeclaration();
		case Token::Type::KeyWhile: return ParseWhileLoop();
		case Token::Type::KeyYield: return ParseYield();
		case Token::Type::EOL: return ParseBlankLine();
		default: return ParseExprStatement();
	}
}

// Statement::ParseAssertion
//
// 'assert' expression
//
AST::Statement *Statement::ParseAssertion()
{
	Expect( Token::Type::KeyAssert );
	AST::Expression *exp = ParseExpression();
	return new AST::Assertion( exp, _indentLevel, Location() );
}

AST::Statement *Statement::ParseBlankLine()
{
	return new AST::BlankLine( _indentLevel, Location() );
}

// Statement::ParseBlockEnd
//
// 'end' [{identifier | 'if' | 'while'}]
//
// The end statement terminates a scope block. You may name the block you
// intend to exit; this allows the parser to detect mismatches and report
// appropriate errors. For multiline declarations, the name is the declaration's
// name; for flow-control statements, the name is the keyword that introduces
// the block.
//
AST::Statement *Statement::ParseBlockEnd()
{
	Expect( Token::Type::KeyEnd );
	Token tk = Current();
	AST::Statement *out = NULL;
	if (Match( Token::Type::KeyIf ) || 
			Match( Token::Type::KeyWhile ) || 
			Match( Token::Type::Identifier )) {
		out = new AST::NamedBlockEnd( tk, _indentLevel, Location() );
	} else {
		if (!IsCurrent( Token::Type::EOL )) {
			SyntaxError( Error::Type::EndExpectsIdentifier );
			NextInputToken();
		}
		out = new AST::AnyBlockEnd( _indentLevel, Location() );
	}
	return out;
}

// Statement::ParseDebugTrae
//
// 'debug_trace' expression
//
// Print a message to stderr: this is a simple debugging tool. It is a cheat
// out of the IO system, since it does not require you to sync from an IO task.
//
AST::Statement *Statement::ParseDebugTrace()
{
	Expect( Token::Type::KeyDebug_Trace );
	AST::Expression *exp = ParseExpression();
	return new AST::DebugTrace( exp, _indentLevel, Location() );
}

// Statement::ParseDefinition
//
// 'def' identifier '=' expression
//
// The def statement has exactly the same grammar as the var statement and does
// almost exactly the same thing: the only difference is that a definition
// can't be updated once it has been declared. This isn't that big a deal from
// a semantic point of view, but it is still valuable because it lets
// programmers express their intentions in a way that the compiler can enforce.
//
AST::Statement *Statement::ParseDefinition()
{
	Expect( Token::Type::KeyDef );
	Token name = ExpectDeclIdent();
	Expect( Token::Type::OpEQ, Error::Type::UnknownDeclarationValue );
	AST::Expression *exp = ParseExpression();
	return new AST::Definition( name, exp, _indentLevel, Location() );
}

// Statement::ParseElse
//
// 'else' ['if' expression] ':'
//
// Else statement. May be followed by an if clause, introducing a subsidiary
// if-expression. Followed by a colon, introducing the statement block.
//
AST::Statement *Statement::ParseElse()
{
	Expect( Token::Type::KeyElse );
	AST::Expression *exp = OptionalExpression( Token::Type::KeyIf );
	Expect( Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::Else( exp, _indentLevel, Location() );
}

// Statement::ParseExprStatement
// 
// exprstatement:
//	target '=' expression
//	target ['(' arguments ')']
//
// The line consists of an expression used as a statement. We'll interpret it
// as some sort of action. Of course the point is to redefine some symbol,
// which is specified by the leading identifier.
//
AST::Statement *Statement::ParseExprStatement()
{
	AST::Expression *var = ParseTarget();
	if (Current().TokenType() == Token::Type::OpEQ) {
		return ParseAssignment( var );
	} else {
		return ParseMutation( var );
	}
}

// Statement::ParseAssignment
//
//	target '=' expression
//
// Give some symbol, or some group of symbols, new meanings from an expression.
//
AST::Statement *Statement::ParseAssignment( AST::Expression *target )
{
	Expect( Token::Type::OpEQ );
	return new AST::Assignment( 
			target, ParseExpression(), _indentLevel, Location() );
}

// Statement::ParseMutation
//
//	target ['(' arguments ')']
//
// Syntactic sugar for an assignment to the result of a function call.
//
AST::Statement *Statement::ParseMutation( AST::Expression *target )
{
	AST::Expression *args =
			OptionalExpression( Token::Type::ParenL, Token::Type::ParenR );
	if (args) args = new AST::Arguments( args, args->Location() );
	return new AST::Mutation( target, args, _indentLevel, Location() );
}

// Statement::ParseTarget
//
// target:	(item ',')* item
//
// Assignable thing. An identifier, or a tuple of identifiers. As usual, we
// expect that the current token is the first token of this production, and
// when we return the current token will be the one following. This will never
// return NULL.
//
AST::Expression *Statement::ParseTarget()
{
	AST::Expression *exp = ParseTargetItem();
	while (Match( Token::Type::Comma )) {
		exp = new AST::OpTuple( exp, ParseTargetItem() );
	}
	return exp;
}

// Statement::ParseTargetItem
//
// item:
//		identifier ['->' identifier]* ['[' expression ']']
//		'(' target ')'	// assign to elements from a sequence
//		'[' target ']'	// assign by looking up numeric indexes
//		'{' target '}'	// assign to vars from a map
//
AST::Expression *Statement::ParseTargetItem()
{
	Token tk = Current();
	AST::Expression *exp = new AST::Dummy( tk );
	if (Match( Token::Type::Identifier )) {
		exp = new AST::Identifier( tk.Value(), tk.Location() );
		while (Match( Token::Type::RightArrow )) {
			tk = Current();
			Expect(
					Token::Type::Identifier,
					Error::Type::StatementExpectsIdentifier );
			AST::Expression *rhs =
					new AST::Identifier( tk.Value(), tk.Location() );
			exp = new AST::Member( exp, rhs, exp->Location() + tk.Location() );
		}
		if (Match( Token::Type::BracketL )) {
			AST::Expression *arg = ParseExpression();
			exp = new AST::Lookup(
					exp, arg, exp->Location() + arg->Location() );
			Expect( Token::Type::BracketR );
		}
	}
	else if (Match( Token::Type::ParenL )) {
		exp = ParseTarget();
		Expect( Token::Type::ParenR );
	}
	else if (Match( Token::Type::BracketL )) {
		AST::Expression *target = ParseTarget();
		exp = new AST::List( target, tk.Location() + Current().Location() );
		Expect( Token::Type::BracketR );
	}
	else if (Match( Token::Type::BraceL )) {
		AST::Expression *target = ParseTarget();
		exp = new AST::Map( target, tk.Location() + Current().Location() );
		Expect( Token::Type::BraceR );
	}
	else {
		SyntaxError( Error::Type::StatementExpectsIdentifier );
	}
	return exp;
}

// Statement::ParseForLoop
//
// 'for' identifier 'in' expression ':'
//
// For loop introduces a counter variable, and accepts a sequence expression.
// Always followed by a colon, which introduces the loop body.
//
AST::Statement *Statement::ParseForLoop()
{
	Expect( Token::Type::KeyFor );
	Token name = ExpectDeclIdent();
	Expect( Token::Type::KeyIn, Error::Type::ForLoopExpectsInKeyword );
	AST::Expression *exp = ParseExpression();
	Expect( Token::Type::Colon, Error::Type::ForLoopExpectsBlockBegin );
	return new AST::ForLoop( name, exp, _indentLevel, Location() );
}

// Statement::ParseFunctionDeclaration
//
// 'function' identifier ['(' expression ')'] {':' | '=' expression}
//
AST::Statement *Statement::ParseFunctionDeclaration()
{
	Expect( Token::Type::KeyFunction );
	Token name = ExpectDeclIdent();
	AST::Expression* parameter =
			OptionalExpression( Token::Type::ParenL, Token::Type::ParenR );
	AST::Expression *expr = OptionalExpression( Token::Type::OpEQ );
	if (!expr) Expect(
			Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::FunctionDeclaration(
			name, parameter, expr, _indentLevel, Location() ); 
}

// Statement::ParseMethodDeclaration
//
// 'method' identifier ['(' expression ')'] ':'
//
AST::Statement *Statement::ParseMethodDeclaration()
{
	Expect( Token::Type::KeyMethod );
	Token name = ExpectDeclIdent();
	AST::Expression* parameter =
			OptionalExpression( Token::Type::ParenL, Token::Type::ParenR );
	Expect( Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::MethodDeclaration(
			name, parameter, _indentLevel, Location() );
}

// Statement::ParseIfThen
//
// 'if' expression ':'
//
// If statement. Get the condition expression, then the block-entry colon. We
// might want to do a single-line if statement someday, in which case the 'Then'
// token would come in place of the colon, followed by some non-declaration
// statement.
//
AST::Statement *Statement::ParseIfThen()
{
	Expect( Token::Type::KeyIf );
	AST::Expression *exp = ParseExpression();
	Expect( Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::IfThen( exp, _indentLevel, Location() );
}

// Statement::ParseImport
//
// 'import' identifier ['from' expression]
//
AST::Statement *Statement::ParseImport()
{
	Expect( Token::Type::KeyImport );
	Token name = ExpectDeclIdent();
	const AST::Expression *source = OptionalExpression( Token::Type::KeyFrom );
	return new AST::ImportDeclaration(
			name, source, _indentLevel, Location() );
}

// Statement::ParseObjectDeclaration
//
// 'object' identifier ['(' expression ')'] ['from' expression] ':'
//
AST::Statement *Statement::ParseObjectDeclaration()
{
	Expect( Token::Type::KeyObject );
	Token name = ExpectDeclIdent();
	AST::Expression *parameter =
			OptionalExpression( Token::Type::ParenL, Token::Type::ParenR );
	const AST::Expression *prototype =
			OptionalExpression( Token::Type::KeyFrom );
	Expect( Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::ObjectDeclaration(
			name, parameter, prototype, _indentLevel, Location() );
}

// Statement::ParseSync
//
// 'sync' expression
//
// Emit a value from an asynchronous task generator.
//
AST::Statement *Statement::ParseSync()
{
	Expect( Token::Type::KeySync );
	AST::Expression *exp = ParseExpression();
	return new AST::SyncStatement( exp, _indentLevel, Location() );
}

// Statement::ParseVarDeclaration
//
// 'var' identifier ['=' expression]
//
AST::Statement *Statement::ParseVarDeclaration()
{
	Expect( Token::Type::KeyVar );
	Token name = ExpectDeclIdent();
	AST::Expression *exp = NULL;
	if (Match( Token::Type::OpEQ )) {
		exp = ParseExpression();
	}
	return new AST::VarDeclaration( name, exp, _indentLevel, Location() ); 
}


// Statement::ParseWhileLoop
//
// 'while' expression ':'
//
// Simple conditional loop. Get the condition expression, then the block-entry
// colon. This is always a block-begin statement.
//
AST::Statement *Statement::ParseWhileLoop()
{
	Expect( Token::Type::KeyWhile );
	AST::Expression *exp = ParseExpression();
	Expect( Token::Type::Colon, Error::Type::UnknownDeclarationValue );
	return new AST::While( exp, _indentLevel, Location() );
}

// Statement::ParseYield
//
// 'yield' ['from'] expression
//
// Emit a value from a sequence generator.
//
AST::Statement *Statement::ParseYield()
{
	Expect( Token::Type::KeyYield );
	bool fromSubseq = Match( Token::Type::KeyFrom );
	AST::Expression *exp = ParseExpression();
	return new AST::Yield( exp, fromSubseq, _indentLevel, Location() );
}

