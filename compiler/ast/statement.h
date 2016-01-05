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

#ifndef ast_statement_h
#define ast_statement_h

#include <assert.h>
#include "ast/expression.h"
#include "ast/binop.h"
#include "error.h"
#include "flowgraph/node.h"

namespace AST {

class Statement
{
	public:
		virtual ~Statement() {}
		SourceLocation Location() const { return _location; }

		// Double-dispatch callbacks: the analyzer invokes this on a statement
		// of unknown class, which then invokes the appropriate method on the
		// analyzer itself.
		virtual void SemGen( class StatementAnalyzer *it ) const = 0;

		virtual bool IsABlankLine() const { return false; }

		// This statement may begin or end a scope block. If so, it must
		// implement BlockName(), so the blockstacker can match begin and end
		// statements.
		virtual bool IsBlockBegin() const { return false; }
		virtual bool IsBlockEnd() const { return false; }
		virtual bool EndsThisBlock( const Token &name ) const { return false; }
		virtual Token BlockName() const
				{ assert( false ); Token dummy; return dummy; }
		virtual bool DelimitsBlock() const { return false; }
		virtual unsigned int IndentLevel() const { return _level; }

	protected:
		Statement( unsigned int level, const SourceLocation &loc );
		unsigned int _level;
		SourceLocation _location;
};

class StatementAnalyzer
{
	public:
		virtual ~StatementAnalyzer() {}
		// We should have one SemGen method for each concrete expression node
		// and for each statement node. This is a double dispatch / visitor
		// system. We should have one method for each statement class. Each one
		// will process the given statement and update the context symbol table.
		virtual void GenAssertion( const class Assertion &it ) = 0;
		virtual void GenAssignment( const class Assignment &it ) = 0;
		virtual void GenBlankLine( const class BlankLine &it ) = 0;
		virtual void GenBlockEnd( const class BlockEnd &it ) = 0;
		virtual void GenDebugTrace( const class DebugTrace &it ) = 0;
		virtual void GenDef( const class Definition &it ) = 0;
		virtual void GenElse( const class Else &it ) = 0;
		virtual void GenForLoop( const class ForLoop &it ) = 0;
		virtual void GenFunction( const class FunctionDeclaration &it ) = 0;
		virtual void GenIfThen( const class IfThen &it ) = 0;
		virtual void GenImport( const class ImportDeclaration &it ) = 0;
		virtual void GenMethod( const class MethodDeclaration &it ) = 0;
		virtual void GenMutation( const class Mutation &it ) = 0;
		virtual void GenObject( const class ObjectDeclaration &it ) = 0;
		virtual void GenSync( const class SyncStatement &it ) = 0;
		virtual void GenVar( const class VarDeclaration &it ) = 0;
		virtual void GenWhile( const class While &it ) = 0;
		virtual void GenYield( const class Yield &it ) = 0;
};

class Assertion : public Statement
{
	public:
		Assertion(
				const Expression *condition,
				unsigned int indent,
				const SourceLocation &loc );
		~Assertion() { delete _condition; }
		const Expression *Condition() const { return _condition; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenAssertion( *this ); }
	private:
		const Expression *_condition;
};

class DebugTrace : public Statement
{
	public:
		DebugTrace(
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~DebugTrace() { delete _exp; }
		const Expression *Exp() const { return _exp; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenDebugTrace( *this ); }
	private:
		const Expression *_exp;
};

class Assignment : public Statement
{
	public:
		Assignment(
				Expression *lhs,
				Expression *rhs,
				unsigned int indent,
				const SourceLocation &loc );
		~Assignment();
		const Expression *Left() const { return _LHS; }
		const Expression *Right() const { return _RHS; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenAssignment( *this ); }
	private:
		Expression *_LHS;
		Expression *_RHS;
};

class Mutation : public Statement
{
	public:
		Mutation(
				Expression *target,
				Expression *args,
				unsigned int indent,
				const SourceLocation &loc );
		~Mutation();
		const Expression *Target() const { return _target; }
		const Expression *Args() const { return _args; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenMutation( *this ); }
	private:
		Expression *_target;
		Expression *_args;
};

class BlankLine : public Statement
{
	public:
		BlankLine(
				unsigned int indent,
				const SourceLocation& loc ) : Statement(indent, loc) {}
		void SemGen( StatementAnalyzer *it) const
				{ it->GenBlankLine( *this ); }
		bool IsABlankLine() const { return true; }
};

class BlockEnd : public Statement
{
	public:
		BlockEnd(
				unsigned int indent,
				const SourceLocation &loc ) : Statement(indent, loc) {}
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenBlockEnd( *this ); }
		virtual bool IsBlockEnd() const { return true; }
		virtual bool EndsThisBlock( const Token &name ) const { return true; }
};

class AnyBlockEnd : public BlockEnd
{
	public:
		AnyBlockEnd(
				unsigned int indent,
				const SourceLocation &loc ) : BlockEnd(indent, loc) {}
		Token BlockName() const { Token dummy; return dummy; }
		bool EndsThisBlock( const Token &name ) const { return true; }
};

class NamedBlockEnd : public BlockEnd
{
	public:
		NamedBlockEnd(
				const Token &name,
				unsigned int indent, const SourceLocation &loc );
		Token BlockName() const { return _name; }
		bool EndsThisBlock( const Token &name ) const
				{ return name.Value() == _name.Value(); }
	private:
		Token _name;
};

class IfThen : public Statement
{
	public:
		IfThen(
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~IfThen() { delete _exp; }
		bool IsAIfThen() const { return true; }
		bool IsBlockBegin() const { return true; }
		Token BlockName() const;
		const Expression *Exp() const { return _exp; }
		void SemGen( StatementAnalyzer *it ) const { it->GenIfThen( *this ); }
	private:
		const Expression *_exp;
};

class Else : public Statement
{
	public:
		Else(
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~Else() { if (_exp) delete _exp; }
		virtual bool DelimitsBlock() const { return true; }
		const Expression *Exp() const { return _exp; }
		void SemGen( StatementAnalyzer *it ) const { it->GenElse( *this ); }
	private:
		const Expression *_exp;
};

class ForLoop : public Statement
{
	public:
		ForLoop(
				Token name,
				const Expression *sequence,
				unsigned int indent,
				const SourceLocation &loc );
		~ForLoop() { delete _exp; }
		bool IsBlockBegin() const { return true; }
		Token BlockName() const { return _name; }
		const Expression *Exp() const { return _exp; }
		void SemGen( StatementAnalyzer *it ) const { it->GenForLoop( *this ); }
	private:
		Token _name;
		const Expression *_exp;
};

class While : public Statement
{
	public:
		While(
				const Expression *condition,
				unsigned int indent,
				const SourceLocation &loc );
		~While() { delete _condition; }
		bool IsBlockBegin() const { return true; }
		Token BlockName() const;
		const Expression *Exp() const { return _condition; }
		void SemGen( StatementAnalyzer *it ) const { it->GenWhile( *this ); }
	private:
		const Expression *_condition;
};

class Yield : public Statement
{
	public:
		Yield(
				const Expression *exp,
				bool fromSubsequence,
				unsigned int indent,
				const SourceLocation &loc );
		~Yield();
		const Expression *Exp() const { return _exp; }
		bool FromSubsequence() const { return _fromSubsequence; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenYield( *this ); }
	private:
		const Expression *_exp;
		bool _fromSubsequence;
};

class SyncStatement : public Statement
{
	public:
		SyncStatement(
				const Expression *exp,
				unsigned int indent,
				const SourceLocation &loc );
		~SyncStatement();
		const Expression *Exp() const { return _exp; }
		void SemGen( StatementAnalyzer *it ) const
				{ it->GenSync( *this ); }
	private:
		const Expression *_exp;
};


} // namespace AST

#endif // statement_h

