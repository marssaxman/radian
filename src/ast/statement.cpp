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

#include <assert.h>
#include "ast/statement.h"

using namespace AST;
using namespace std;

Statement::Statement( unsigned int indent, const SourceLocation &loc ):
	_level(indent),
	_location(loc)
{
}

Assertion::Assertion(
		const Expression *condition,
		unsigned int indent,
		const SourceLocation &loc ) :
	Statement(indent, loc),
	_condition(condition)
{
	assert(condition);
}

DebugTrace::DebugTrace(
		const Expression *exp, unsigned indent, const SourceLocation &loc ) :
	Statement(indent, loc),
	_exp(exp)
{
	assert(exp);
}

Assignment::Assignment(
		Expression *lhs,
		Expression *rhs,
		unsigned int indent,
		const SourceLocation &loc ) :
	Statement(indent, loc),
	_LHS(lhs),
	_RHS(rhs)
{
	assert( lhs );
	assert( rhs );
}

Assignment::~Assignment()
{
	delete _LHS;
	delete _RHS;
}

Mutation::Mutation(
		Expression *target,
		Expression *args,
		unsigned int indent,
		const SourceLocation &loc ) :
	Statement(indent, loc),
	_target(target),
	_args(args)
{
	assert( target );
}

Mutation::~Mutation()
{
	delete _target;
	if (_args) delete _args;
}

NamedBlockEnd::NamedBlockEnd(
		const Token &name,
		unsigned int indent,
		const SourceLocation &loc ) :
	BlockEnd(indent, loc),
	_name(name)
{
}

IfThen::IfThen(
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_exp(exp)
{
	assert( exp );
}

Token IfThen::BlockName() const
{
	// Every block must have a name, so the blockstacker can match ends with
	// beginnings. Declaration blocks are each named after their symbol, but
	// every if block is named "if". We will dummy up a keyword token 
	// representing "if" and return that as this block's name. We expect that
	// the parser will perform the same trick when handling the "end if"
	// statement, so the names will match normally. It'd be nicer if we could
	// have the token class look the string up from its own table...
	string ifname = "if";
	SourceLocation loc = _exp->Location();
	Token out( ifname, Token::Type::KeyIf, loc );
	return out;
}

Else::Else(
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_exp(exp)
{
	// The condition clause is optional for an else statement, so don't
	// assert(exp).
}

ForLoop::ForLoop(
		Token name,
		const Expression *sequence,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_name(name),
	_exp(sequence)
{
	assert( sequence );
}

While::While(
		const Expression *condition,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_condition(condition)
{
	assert( condition );
}

Token While::BlockName() const
{
	// Every block must have a name, so the blockstacker can match ends with
	// beginnings. Every while block is called "while".
	string whilename = "while";
	Token out( whilename, Token::Type::KeyWhile, Location() );
	return out;
}

SyncStatement::SyncStatement(
		const Expression *exp,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_exp(exp)
{
}

SyncStatement::~SyncStatement()
{
	if (_exp) delete _exp;
}

Yield::Yield(
		const Expression *exp,
		bool fromSubsequence,
		unsigned int indent,
		const SourceLocation &loc ):
	Statement(indent, loc),
	_exp(exp),
	_fromSubsequence(fromSubsequence)
{
	assert( _exp );
}

Yield::~Yield()
{
	delete _exp;
}
