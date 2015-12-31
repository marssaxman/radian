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


#include "scope.h"
#include "expressionanalyzer.h"
#include "statementanalyzer.h"

using namespace Semantics;
using namespace Flowgraph;

Scope::Scope( Pool &pool ):
	_pool(pool),
	_segments(NULL)
{
}

Scope::~Scope()
{
	// Regardless of what happened during the lifetime of the scope, we must
	// end up with no segments. This is a sign that the subclass has correctly
	// packaged the segment chain and has not simply ignored it. Failing to
	// deal with the segment chain will produce a meaningless function result,
	// so this is very important.
	assert( NULL == _segments );
}

// Scope::Eval
//
// Get a value for this expression, resolving identifiers in the current scope.
//
Node *Scope::Eval( const AST::Expression *it )
{
	ExpressionAnalyzer engine( _pool, *this );
	engine.ProcessSyncs( it );
	return engine.Eval( it );
}

// Scope::SemGen
//
// Process this statement in the current scope. Statements may change scope, so
// we must return the new "current" scope after the statement is processed.
//
Scope *Scope::SemGen( const AST::Statement *it )
{
	StatementAnalyzer engine( _pool, this );
	return engine.SemGen( it );
}

// Scope::Resolve
//
// Given a symbol, find out whether it is defined, and if so, retrieve its
// value and kind.
//
Symbol Scope::Resolve( Node *name )
{
	// Is the symbol defined in our scope's current table?
	Symbol sym = _symbols.Lookup( name );

	// Was the smybol defined in a previous segment of this scope?
	if (!sym.IsDefined()) sym = RetrieveFromPreviousSegment( name );

	// Is there a definition in our context we can capture?
	if (!sym.IsDefined()) sym = RetrieveFromContext( name );
	
	return sym;
}

// Scope::Define
//
// Try to create a symbol with this name in the current scope. If the symbol is
// already defined, we'll register an error; otherwise we'll define the symbol.
//
void Scope::Define(
		Node *nameSym,
		Node *value,
		Symbol::Type::Enum kind,
		const SourceLocation &loc )
{
	assert( nameSym );
	assert( value );
	if (_definitions.find( nameSym ) != _definitions.end()) {
		ReportError( Error::Type::AlreadyDefined, loc );
	} else {
		_definitions[nameSym] = kind;
		_symbols.Insert( nameSym, Symbol( kind, value ) );
	}
}

// Scope::Assign
//
// Assign this value to that identifier symbol. If the symbol is not assignable,
// or not defined, or for some other reason cannot be assigned to, report an
// error. If there is no definition for the symbol in our local scope, we will
// attempt to rebind the context value instead.
//
void Scope::Assign( Node *nameSym, Node *val, const SourceLocation &loc  )
{
	assert( nameSym );
	assert( val );
	Error::Type::Enum response = Error::Type::None;
	// Attempt to assign to a local definition of this symbol. If we succeed,
	// that's all there is to it. Otherwise, if the symbol is not currently
	// defined, look for a definition in a previous segment or a containing
	// scope and try to assign there.
	if (_symbols.Update( nameSym, val, &response )) {
		// We succeeded! Job finished.
		assert( response == Error::Type::None );
	} else if (Error::Type::Undefined == response) {
		response = AssignToUndefined( nameSym, val, loc );
	}
	// If we failed the assignment, report the error. Otherwise, if the var we
	// just assigned to was captured from a context scope, we must report that
	// assignment to the subclass so it can maintain its rebind list.
	if (response != Error::Type::None) {
		ReportError( response, loc );
	} else if (_wasCaptured.find(nameSym) != _wasCaptured.end()) {
		RebindInContext( nameSym, val, loc );
	}
}

Scope *Scope::PartitionElse( const AST::Else &it )
{
	ReportError( Error::Type::ElseStatementOutsideIfBlock, it.Location() );
	return this;
}

// Scope::PushSegment
//
// Divide this scope into multiple asynchronous segments. The segment will
// capture the current state of all symbols plus some result value, and we will
// chain these up so that each segment remembers the one that preceded it.
// We will then clear the local symbol table, since we are now operating in a
// new evaluation context, even if it is the same semantic scope. We can make
// existing symbols accessible by capturing them into the new segment from the
// previous one.
//
void Scope::PushSegment(
		Node *value, Segment::Type::Enum type, const SourceLocation &loc )
{
	if (NeedsImplicitSelf()) {
		ReportError( Error::Type::YieldInsideMemberDispatch, loc );
		return;
	}
	if (_segments) {
		if (type == Segment::Type::Sync && !_segments->Synchronization()) {
			ReportError( Error::Type::SyncInsideGenerator, loc );
		}
		if (_segments->Synchronization() && type != Segment::Type::Sync) {
			ReportError( Error::Type::YieldInsideAsyncTask, loc );
		}
	}

	_segments = new Segment( _pool, _segments, _symbols, value, type );
	_symbols.Clear();
}

// Scope::SegmentsSynchronize
//
// Does this scope contain synchronizing segments, from an async task?
// This is obviously not true if the scope contains no segments, but it also
// requires that the yields between segments are performed with a 'sync'
// rather than a 'yield'.
//
bool Scope::SegmentsSynchronize() const
{
	if (!_segments) return false;
	return _segments->Synchronization();
}

// Scope::PackageSegmentedResult
//
// When a block has finished, and is ready to return its result, it must wrap
// that result up in the chain of segments which created the result. If there
// were any segments, that is - if this scope is unsegmented, there is no need
// to produce a generator around the result. Up to you to know what you want.
//
Node *Scope::PackageSegmentedResult( Node *result )
{
	// Make a terminal iterator around this result value.	
	Node *core = _pool.ImportCore();
	Node *make_terminator = _pool.Call1( core, _pool.Sym_Make_Terminator() );
	result = _pool.Call2( make_terminator, core, result );

	if (_segments) {
		// Tell the previous segment to wrap itself around the terminal we have
		// constructed. It will recurse until the beginning of the chain, returning
		// an iterator pointing at the first value of the sequence.	We will then
		// destroy the segment chain, since we have applied it, which guarantees
		// that we will not package the segments twice and lets the destructor
		// ensure that the segments have been dealt with before we exit scope.
		result = _segments->Iterator( result );
		delete _segments;
		_segments = NULL;
	}

	// Wrap the chain in a sequence object, which can be iterated over.
	// That is our new result.
	Node *make_sequence = _pool.Call1( core, _pool.Sym_Make_Seq_Or_Task() );
	Node *sequence = _pool.Call2( make_sequence, core, result );
	return sequence;
}

// Scope::RetrieveFromPreviousSegment
//
// We are trying to resolve a symbol which is not in our active table. It may
// have been defined in a previous segment; if so, we will retrieve it and 
// insert it into the active table. If not, we will return NULL, and the
// resolve process will continue.
//
Symbol Scope::RetrieveFromPreviousSegment( Node *name )
{
	// Has this scope ever defined the symbol in question? If not, there is no
	// previous segment value for us to retrieve.
	if (_definitions.find( name ) == _definitions.end()) {
		return Symbol::Undefined();
	}
	// If, on the other hand, we *have* defined it somewhere, we must therefore
	// have a segment, which will be able to provide a value.
	assert( _segments );
	Symbol sym = _segments->Resolve( name );
	assert( sym.IsDefined() );
	// We don't need to retrieve the value twice, so we'll stash it in our
	// active symbols list, where we can easily find it next time.
	_symbols.Insert( name, sym );
	return sym;
}

// Scope::RetrieveFromContext
//
// We are trying to resolve a symbol which is not in our active symbol table
// and has never been defined in this scope. It may have been defined in an
// outer scope; if so, we will retrieve it and insert it into the active table.
//
Symbol Scope::RetrieveFromContext( Node *name )
{
	Symbol sym = CaptureFromContext( name );
	if (sym.IsDefined()) {
		_wasCaptured.insert( name );
		_definitions[name] = sym.Kind();
		if (_segments) {
			_segments->PropagateCapturedValue( name, sym );
			sym = _segments->Resolve( name );
		}
		_symbols.Insert( name, sym );
	}
	return sym;
}

// Scope::AssignToUndefined
//
// If we did not succeed in updating a local definition, we may be trying to
// update a symbol defined in an earlier segment, or we may be rebinding a free
// variable captured from context. We will only try this if the reason we
// didn't succeed was that the symbol is undefined; that is, if this scope
// contains a definition for a symbol, there is no way to redefine the symbol
// in any containing scope.
//
Error::Type::Enum Scope::AssignToUndefined(
		Node *nameSym, Node *value, const SourceLocation &loc )
{
	std::map<Node*,Symbol::Type::Enum>::const_iterator found =
			_definitions.find( nameSym );
	if (found != _definitions.end()) {
		// We have encountered this symbol before, but we have never pulled its
		// value up to the current segment. Since we're now replacing the value
		// we will never need to pull that value up. 
		_symbols.Insert( found->first, Symbol( found->second, _pool.Nil() ) );
	} else {
		// We have never encountered this symbol before in this scope. If it
		// exists at all, it must be defined by our context. Go try to capture
		// it, then insert it into the current symbol table.
		RetrieveFromContext( nameSym );
	}
	Error::Type::Enum response = Error::Type::None;
	_symbols.Update( nameSym, value, &response );
	return response;
}

// Scope::RewriteSegmentCaptures
//
// The loop system generates placeholders for captured context values, then
// rewrites them at the end when we know whether we've reassigned those symbols
// or only used their captured values. Go pass this rewrite information along
// to the segments, if we have them, so they can rewrite their capture lists.
//
void Scope::RewriteSegmentCaptures( Flowgraph::NodeMap &reMap )
{
	if (_segments) {
		_segments->RewriteCapturedValues( reMap );
	}
}

Layer::Layer( Pool &pool, Scope *context ):
	Scope(pool),
	_context(context)
{
	assert( context );
}

// Layer::CaptureFromContext
//
// We have encountered a reference to a symbol which is not defined in the
// local scope. Retrieve its value from our context and create a local
// reference to the value. The caller will insert this value into our symbol
// table.
//
Symbol Layer::CaptureFromContext( Node *name )
{
	Symbol sym = Context().Resolve( name );
	Node *value = sym.IsDefined() ? sym.Value() : NULL;
	if (value) value = CreateLocalReference( name, value );
	return Symbol( sym.Kind(), value );
}

