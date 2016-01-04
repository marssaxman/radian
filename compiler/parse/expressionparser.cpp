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

#include "parse/expressionparser.h"
#include "main/error.h"

using namespace Parser;

// Expression::Expression
//
// Main entrypoint for expression parsing. Including the current input token,
// parse a complete expression. When we return, the current token will be the
// token which immediately follows the expression, as with all of the other
// expression-parsing methods. Ignoring precedence, we can model our expression
// grammar like this:
//     term {op term}*
// The traditional recursive-descent parser embeds knowledge about precedence
// into the parser structure, but it turns out to be much simpler to just
// ignore precedence in the parser and then apply a simple recursive graph
// reorganization when necessary.
//
Expression::Expression( Iterator<Token> &input, Reporter &log ):
	Core( input, log ),
	_result(NULL)
{
	_result = ParseTerm();
	while (AST::BinOp::IsBinOpToken( Current() )) {
		Token tk = Current();
		NextInputToken();
		SkipOptionalLinebreak();
		_result = AST::BinOp::Create( _result, tk, ParseTerm() );
		_result = _result->Reassociate();
	}
	assert( _result );
}

// Expression::SkipOptionalLinebreak
//
// Radian is basically line-structured, but we allow non-statement-ending line
// breaks at certain points inside expressions. This enables the programmer to
// split long statements across multiple lines instead of breaking them up into
// separate statements, which is often better but not always better.
// You must call this function after parsing any binop token or after parsing
// any subexpression-grouping token (paren, bracket, brace).
//
void Expression::SkipOptionalLinebreak()
{
	if (Match( Token::Type::EOL )) {
		NextInputToken();
		// It would be nice to validate that the continuing line's indentation
		// level is greater than the current nominal indent level, but that can
		// wait for future work.
		while (IsCurrent( Token::Type::Indent )) {
			NextInputToken();
		}
	}
}

// Expression::ParseArguments
//
// This is just a placeholder that lets us evaluate an expression in an
// argument context, so we can produce an arg list instead of a tuple instance
// when we encounter the comma operator.
//
AST::Arguments *Expression::ParseArguments()
{
	AST::Expression *out = ParseExpression();
	return new AST::Arguments( out, out->Location() );
}

// ExpressionParser::ParseCapture
//
// capture:	'capture' '(' [exp ':'] exp ')'
//
// Create a function reference/object from the expression, capturing the
// current values of any context symbols it uses. If there is a parameter list,
// the expression can use these names as its parameters.
//
AST::Expression *Expression::ParseCapture()
{
	Expect( Token::Type::ParenL );
	AST::Expression *param = ParseExpression();
	AST::Expression *exp = OptionalExpression( Token::Type::Colon );
	if (!exp) {
		exp = param;
		param = NULL;
	}
	Expect( Token::Type::ParenR );
	return new AST::Lambda( exp, param, Location() );
}

// Expression::ParseEvaluation
//
// evaluation: identifier [ '(' arguments ')' ]
//
// An evaluation has an identifier and may have a subscript containing an
// argument expression. Unlike C-style languages, the subscript may not be
// empty, since its presence does not distinguish between invocation and
// reference (naming always implies invocation).
//
AST::Expression *Expression::ParseEvaluation( const Token &tk )
{
	assert( tk.TokenType() == Token::Type::Identifier );
	AST::Expression *out = new AST::Identifier( tk.Value(), tk.Location() );
	if (Match( Token::Type::ParenL )) {
		out = new AST::Call( out, ParseArguments(), Location() );
		Expect( Token::Type::ParenR );
	}
	return out;
}

// Expression::ParseInvoke
//
// invoke:	'invoke' '(' exp [':' exp] ')'
//
// Invoke an object/function reference, with optional arguments.
//
AST::Expression *Expression::ParseInvoke()
{
	Expect( Token::Type::ParenL );
	AST::Expression *exp = ParseExpression();
	AST::Expression *args = OptionalExpression( Token::Type::Colon );
	if (args) args = new AST::Arguments( args, args->Location() );
	Expect( Token::Type::ParenR );
	return new AST::Invoke( exp, args, Location() );
}

// Expression::ParseList
//
// array: '[' expression ']'
//
// Create an ordered list from a series of expressions.
//
AST::Expression *Expression::ParseList( const Token &lbracket )
{
	assert( lbracket.TokenType() == Token::Type::BracketL );
	AST::Expression *out = ParseExpression();
	Expect( Token::Type::BracketR );
	return new AST::List( out, Location() );
}

// Expression::ParseListComprehension
//
// Map and/or filter some sequence.
//
AST::Expression *Expression::ParseListComprehension()
{
	AST::Expression *output = ParseExpression();
	AST::Expression *variable = OptionalExpression( Token::Type::KeyFrom );
	if (!variable) {
		variable = output;
		output = NULL;
	}
	Expect( Token::Type::KeyIn );
	AST::Expression *input = ParseExpression();
	AST::Expression *predicate = OptionalExpression( Token::Type::KeyWhere );
	return new AST::ListComprehension(
			output, variable, input, predicate, Location() );
}

// Expression::ParseMap
//
// map: '{' expression '}'
//
// Map is an associative array. The element should be either a key-value pair
// or just a value; if you omit the value, the value doubles as its own key,
// allowing the map to serve dual duty as a set.
//
AST::Expression *Expression::ParseMap( const Token &lbrace )
{
	assert( lbrace.TokenType() == Token::Type::BraceL );
	AST::Expression *out = ParseExpression();
	Expect( Token::Type::BraceR );
	return new AST::Map( out, Location() );
}

// Expression::ParsePrimary
//
// primary:
//	evaluation
//	'-' term
//	'not' term
//	number
//	string
//	'true'
//	'false'
//	subexpression
//	list
//	map
//	invoke
//	capture
//	'sync' ['(' exp ')']
//	'throw' '(' exp ')'
//	_dump( exp )
//
AST::Expression *Expression::ParsePrimary()
{
	Token tk = Current();
	NextInputToken();
	Error::Type::Enum err = Error::Type::None;
	switch (tk.TokenType()) {
		case Token::Type::Identifier: return ParseEvaluation( tk );

		// Literals
		case Token::Type::Symbol: return new AST::SymbolLiteral( tk );
		case Token::Type::Integer: return new AST::IntegerLiteral( tk );
		case Token::Type::Real: return new AST::RealLiteral( tk );
		case Token::Type::Float: return new AST::FloatLiteral( tk );
		case Token::Type::Hex: return new AST::HexLiteral( tk );
		case Token::Type::Oct: return new AST::OctLiteral( tk );
		case Token::Type::Bin: return new AST::BinLiteral( tk );
		case Token::Type::String: return new AST::StringLiteral( tk );
		case Token::Type::KeyTrue: return new AST::BooleanLiteral( tk );
		case Token::Type::KeyFalse: return new AST::BooleanLiteral( tk );

		// Unary operators
		case Token::Type::Minus:
			return new AST::NegationOp( ParseTerm(), Location() );
		case Token::Type::KeyNot:
			return new AST::NotOp( ParseTerm(), Location() );
		case Token::Type::Poison:
			return new AST::Throw( ParseTerm(), Location() );

		// Containers
		case Token::Type::ParenL: return ParseSubExpression( tk );
		case Token::Type::BracketL: return ParseList( tk );
		case Token::Type::BraceL: return ParseMap( tk );

		// Compound non-precedential operators
		case Token::Type::KeyInvoke: return ParseInvoke();
		case Token::Type::KeyCapture: return ParseCapture();
		case Token::Type::KeySync: return ParseSync();
		case Token::Type::KeyThrow: return ParseThrow();
		case Token::Type::KeyEach: return ParseListComprehension();

		// Error cases
		case Token::Type::ParenR: err = Error::Type::EmptySubexpression; break;
		case Token::Type::BracketR: err = Error::Type::EmptyList; break;
		case Token::Type::BraceR: err = Error::Type::EmptyMap; break;
		default: err = Error::Type::UnknownExpressionToken;
	}
	SyntaxError( err );
	return new AST::Dummy( tk );
}

// Expression::ParseSubExpression
//
// subexpression: '(' expression ')'
//
// A subexpression is simply a pair of parens around another expression.
// This lets you override precedence, just as in mathematics.
//
AST::Expression *Expression::ParseSubExpression( const Token &lparen )
{
	assert( lparen.TokenType() == Token::Type::ParenL );
	AST::Expression *out = ParseExpression();
	Expect( Token::Type::ParenR );
	return new AST::SubExpression( out, Location() );
}

// ExpressionParser::ParseSync
//
// sync: 'sync' ['(' exp ')']
//
// Yield some value back to the controlling process, then return with whatever
// next value it supplies us with.
//
AST::Expression *Expression::ParseSync()
{
	AST::Expression *exp = NULL;
	if (Match( Token::Type::ParenL )) {
		exp = ParseExpression();
		Expect( Token::Type::ParenR );
	}
	return new AST::Sync( exp, Location() );
}

// Expression::ParseTerm
//
// term: primary postfix*
// postfix: '[' arguments ']' | '.' evaluation
//
AST::Expression *Expression::ParseTerm()
{
	AST::Expression *out = ParsePrimary();
	while (true) {
		if (Match( Token::Type::BracketL )) {
			AST::Expression *args = ParseExpression();
			Expect( Token::Type::BracketR );
			out = new AST::Lookup( out, args, Location() );
		} else if (Match( Token::Type::Period )) {
			SkipOptionalLinebreak();
			Token tk = Current();
			if (Expect( Token::Type::Identifier )) {
				out = new AST::Member( out, ParseEvaluation( tk ), Location() );
			}
			else break;
		} else if (Match( Token::Type::RightArrow )) {
			// This is not legal but it is a common mistake and deserves a more
			// specific error message, since we know what they probably meant.
			// Who knows, we may even make it work some day.
			SyntaxError( Error::Type::MutatorInsideExpression );
			SkipOptionalLinebreak();
			Token tk = Current();
			if (Expect( Token::Type::Identifier )) {
				out = new AST::Member( out, ParseEvaluation( tk ), Location() );
			}
			else break;
		}
		else break;
	}
	return out;
}

// ExpressionParser::ParseThrow
//
// throw: 'throw' '(' exp ')'
//
// Wrap some value in an exception object.
//
AST::Expression *Expression::ParseThrow()
{
	Expect( Token::Type::ParenL );
	AST::Expression *exp = ParseExpression();
	Expect( Token::Type::ParenR );
	return new AST::Throw( exp, Location() );
}


