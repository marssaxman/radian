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
// Radian.  If not, see <http://www.gnu.org/licenses/>.


#include "parsercore.h"
#include "expressionparser.h"
#include "statementparser.h"
#include "error.h"

using namespace Parser;

Core::Core( Iterator<Token> &input, Reporter &log ):
	_errors(log),
	_input(input)
{
}

// Core::ParseStatement
//
// Get a statement, starting with the current input token.
//
AST::Statement *Core::ParseStatement()
{
	Parser::Statement engine( _input, _errors );
	return engine.Result();
}

// Core::ParseExpression
//
// Get an expression, starting with the current input token. 
//
AST::Expression *Core::ParseExpression()
{
	Parser::Expression engine( _input, _errors );
	return engine.Result();
}

// Core::OptionalExpression
//
// If we match the flag token, parse and return an expression. Otherwise,
// return NULL. It might be interesting to return some null expression object,
// instead of actual NULL.
//
AST::Expression *Core::OptionalExpression( Token::Type::Enum flag )
{
	if (Match( flag )) {
		return ParseExpression();
	} else {
		return NULL;
	}
}

// Core::OptionalExpression
//
// If we match the begin token, parse an expression, then match the end token.
// Otherwise, return NULL. Again, it might beinteresting to return some null 
// expression object instead of actual NULL. Note that we only require the end
// token when we have found the begin.
//
AST::Expression *Core::OptionalExpression( 
		Token::Type::Enum begin, Token::Type::Enum end )
{
	if (Match( begin )) {
		AST::Expression *out = ParseExpression();
		Expect( end );
		return out;
	} else {
		return NULL;
	}
}

// Core::Next
//
// Wrapper around the next token input. Records the starting location. This is
// how the Location method can automagically capture the span of tokens
// involved in the current production.
//
bool Core::Next()
{
	bool out = _input.Next();
	if (!_startLoc.IsDefined()) {
		_startLoc = Current().Location();
	}
	return out;
}

// Core::NextInputToken
//
// Simple utility function saves us from having to check the NextToken result
// value all over the place. Only call Next() if EOF is an acceptable result,
// which would only be the case at the beginning of a statement line. In all
// other cases, call NextInputToken instead.
//
bool Core::NextInputToken()
{
	bool out = Next();
	if (!out) {
		SyntaxError( Error::Type::UnexpectedEOF );
	}
	else if (IsCurrent( Token::Type::Error )) {
		SyntaxError( Error::Type::BadToken );
		return NextInputToken();
	}
	else if (IsCurrent( Token::Type::Unknown )) {
		SyntaxError( Error::Type::UnknownToken );
		return NextInputToken();
	}
	return out;
}

// Core::IsCurrent
//
// Is the current token's type equal to the one we are curious about?
//
bool Core::IsCurrent( Token::Type::Enum tk )
{
	return Current().TokenType() == tk;
}

// Core::Match
//
// Does the current token match the one we are looking for? If so, consume it,
// and return whatever the result of NextInputToken was.
//
bool Core::Match( Token::Type::Enum tk )
{
	return IsCurrent( tk ) ? NextInputToken() : false;
}

#include <stdio.h>

// Core::Expect
//
// We expect to find a certain token here. If we find it, we will consume it,
// by returning the result of NextInputToken. If we don't find it, we will
// report an error and return false.
//
bool Core::Expect( Token::Type::Enum tk, Error::Type::Enum err )
{
	if (IsCurrent( tk )) {
		return NextInputToken();
	} else {
		// The caller supplies an error to report if we don't match the token
		// we expected, so that we can report the most helpful possible error
		// message. In the case that the token itself is erroneous, however -
		// malformed in some way - the more useful error focuses on the token
		// rather than its context.
		switch (Current().TokenType()) {
			case Token::Type::Error: err = Error::Type::BadToken; break;
			case Token::Type::Unknown: err = Error::Type::UnknownToken; break;
			default: break; // Use whatever error code the caller provided.
		}
		SyntaxError( err );
		return false;
	}
}

// Core::Expect
//
// Simplified version of expect with some generic errors based on the token type.
//
bool Core::Expect( Token::Type::Enum tk )
{
	Error::Type::Enum err = Error::Type::None;
	switch (tk) {
		case Token::Type::ParenL: err = Error::Type::MissingLeftParen; break;
		case Token::Type::ParenR: err = Error::Type::MissingRightParen; break;
		case Token::Type::BracketR: err = Error::Type::MissingRightBracket; break;
		case Token::Type::BraceR: err = Error::Type::MissingRightBrace; break;
		default: err = Error::Type::UnknownExpressionToken;
	}
	return Expect( tk, err );
}

// Core::Synchronize
//
// We expect to find a certain token here. If we don't find it, keep munching
// tokens until we either run out of input or find that token.
//
void Core::Synchronize( Token::Type::Enum tk, Error::Type::Enum err )
{
	if (IsCurrent( tk )) return;
	SyntaxError( err );
	while (Next()) {
		if (IsCurrent( tk )) return;		
	}
}

// Core::SyntaxError
//
// Report an error about the current token.
//
void Core::SyntaxError( Error::Type::Enum err )
{
	_errors.Report( Error( err, Location() ) );
}

// Core::Location
// 
// What is the location of the current production?
//
SourceLocation Core::Location() const
{ 
	return _startLoc + _input.Current().Location();
}


