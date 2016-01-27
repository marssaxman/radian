// Copyright 2012-2016 Mars Saxman.
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

#include "semantics/segment.h"

using namespace Semantics;
using namespace Flowgraph;

Segment::Segment(
		Pool &pool,
		Segment *previous,
		const SymbolTable &symbols,
		Node *value,
		Type::Enum type ):
	_pool(pool),
	_previous(previous),
	_symbols(symbols),
	_value(value),
	_type(type)
{
}

Segment::~Segment()
{
	if (_previous) delete _previous;
}

Symbol Segment::Resolve( Node *name )
{
	assert( name && name->IsASymbol() );
	// The next segment wants the value of this symbol. That is, they want the
	// value that it will have in the next segment's evaluation context, not
	// the value that it had in the previous context. The next segment will be
	// a closure, and this symbol will be one of its captured slots. We will
	// therefore add the current value of this symbol to the capture list we
	// will use later, and then return some appropriate slot reference which
	// the next segment can use in place of the previous value. 
	
	// If we have already encountered this symbol, we will already have a slot
	// reference, which we can simply return.
	Symbol sym = _nextSegmentSlotRefs.Lookup( name );
	if (sym.IsDefined()) return sym;
	
	// This is the first time our context has asked us for this particular
	// value. Look it up in our symbol table. If we didn't happen to have this
	// symbol already, we will in turn ask the previous segment for its value.
	// Once retrieved, we'll store the value in our own symbol table, so we
	// don't have to go through this rigmarole more than once.
	sym = _symbols.Lookup( name );
	if (_previous && !sym.IsDefined()) {
		sym = _previous->Resolve( name );
		assert( sym.IsDefined() );
		_symbols.Insert( name, sym );
	}
	// The next segment must never call this function unless it is certain that
	// the var already exists; it is not a speculative resolution. We can
	// therefore be certain that we will have eventually retrieved a value.
	assert( sym.IsDefined() );
	
	if (!sym.Value()->IsContextIndependent()) {
		// Which slot number will hold this value? We can tell by examining the
		// current length of the capture list.
		unsigned slot_index = _nextSegmentCaptures.size();
		// Add the stored value to the list of capture values, which we'll use
		// to create the closure operation later.
		_nextSegmentCaptures.push( sym.Value() );
		// Create a reference to this slot. This is how the segment which is
		// calling back to us will refer to the value within its own execution
		// context. Each segment keeps track of the next segment's slots.
		sym = Symbol( sym.Kind(), _pool.Slot( slot_index ) );
	}
	_nextSegmentSlotRefs.Insert( name, sym );
	return sym;
}

// Segment::PropagateCapturedValue
//
// The scope has captured a value from its context, but not from the initial
// segment. Since the captured value is only available from the initial segment,
// at the point when the function is first invoked, we must push the capture
// operation back to the head of the chain.
//
void Segment::PropagateCapturedValue( Node *name, Symbol sym )
{
	if (_previous) {
		_previous->PropagateCapturedValue( name, sym );
	} else {
		_symbols.Insert( name, sym );
	}
}

// Segment::Iterator
//
// Return an iterator chain for these segments. Start by wrapping the current
// value and the next value into an object, then - if there was a previous
// segment - have it wrap us up in turn. The final result will be an iterator
// object which contains the whole segment chain. 
// This is where we capture all those slot values.
//
#include <iostream>
Node *Segment::Iterator( Node *next_iterator )
{
	// This function expects its parameter to be an expression yielding an
	// iterator, from the perspective of the next segment. We will turn it into
	// a thunk: doing that means we can evaluate the iterator chain lazily.
	Node *nextfunc = _pool.Function( next_iterator, 1 );
	Node *captures = _pool.Nil();
	while (!_nextSegmentCaptures.empty()) {
		captures = _pool.ArgsAppend( captures, _nextSegmentCaptures.front() );
		_nextSegmentCaptures.pop();
	}
	if (!captures->IsVoid()) {
		nextfunc = _pool.CaptureN( nextfunc, captures );
	}

	// Now that we have a thunked version of the next iterator in the sequence,
	// let's make an iterator for the current segment. We'll use a helper from
	// core which returns an iterator object.
	Node *core = _pool.ImportCore();
	Node *value = _value;
	Node *maker_sym = Sym_Make_Next();
	if (_type == Segment::Type::YieldFrom || _type == Segment::Type::Sync) {
		// our value represents a sequence of values, which we will emit from
		// our output stream, not a single value. This calls for a slightly
		// different kind of iterator.
		Node *iterate_func = _pool.Call1( value, Sym_Begin() );
		value = _pool.Call1( iterate_func, value );
		maker_sym = Sym_Make_Sub();
	}
	Node *make_iterator = _pool.Call1( core, maker_sym );
	Node *result = _pool.Call3( make_iterator, core, value, nextfunc );

	// If there is a previous segment, pass our iterator back to it, so it can
	// perform the same wrapping process. Otherwise, we'll return our own
	// iterator. Either way, when Scope calls ->Iterator on its segment chain,
	// it gets back an iterator pointing at the first item in the sequence.
	return _previous ? _previous->Iterator( result ) : result;
}

// Segment::RewriteCapturedValues
//
// The loop system generates placeholders for captured context values, then
// rewrites them at the end when we know whether we've reassigned those symbols
// or only used their captured values. By the time we've reached the end of the
// loop, we may have a bunch of segments in the way, and the captured values
// are actually defined in the first segment. We'll push the rewrite back to
// the first segment, so the correct values get propagated down the chain of
// async functions.
//
void Segment::RewriteCapturedValues( NodeMap &reMap )
{
	if (_previous) {
		_previous->RewriteCapturedValues( reMap );
	} else {
		std::queue<Node*> excap;
		while (!_nextSegmentCaptures.empty()) {
			Node *exp = _nextSegmentCaptures.front();
			exp = Flowgraph::Rewrite( exp, _pool, reMap );
			_nextSegmentCaptures.pop();
			excap.push( exp );
		}
		_nextSegmentCaptures = excap;
		_value = Flowgraph::Rewrite( _value, _pool, reMap );
	}
}

Node *Segment::Sym_Make_Next() const
{
	return _type == Segment::Type::Sync ?
			_pool.Sym_Make_Action() : _pool.Sym_Make_Iterator();
}

Node *Segment::Sym_Begin() const
{
	return _type == Segment::Type::Sync ?
			_pool.Sym_Start() : _pool.Sym_Iterate();
}

Node *Segment::Sym_Make_Sub() const
{
	return _type == Segment::Type::Sync ?
			_pool.Sym_Make_Subtask() : _pool.Sym_Make_Subsequence();
}
