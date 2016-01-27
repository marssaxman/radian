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
#include "semantics/symbols.h"
#include "flowgraph/pool.h"

using namespace Semantics;
using namespace std;
using namespace Flowgraph;

Symbol::Symbol( Symbol::Type::Enum kind, Flowgraph::Node *value ) :
	_kind(kind),
	_value(value)
{
	assert( (kind == Symbol::Type::Undefined) == (value == NULL) );
}

SymbolTable::SymbolTable( const SymbolTable &src ):
	_items(src._items)
{
}

void SymbolTable::Insert( Node *name, Symbol sym )
{
	assert( name && name->IsASymbol() );
	assert( sym.IsDefined() );
	string tempstr = name->AsValue()->Contents();
	assert( _items.find( tempstr ) == _items.end() );
	_items[tempstr] = sym;
}

bool SymbolTable::Update( Node *name, Node *value, Error::Type::Enum *response )
{
	// Attempt to update the value of the given symbol. If the symbol was
	// defined to accept updates, we will change the value associated with the
	// name, set the response to no error, and return true. If the symbol
	// has been defined not to accept updates, we will return false and set the
	// response to an appropriate  error code. Do not update symbols which
	// don't already exist: use Insert to create them.
	assert( name && name->IsASymbol() );
	assert( value );
	assert( response );
	map<string, Symbol>::iterator iter =
			_items.find( name->AsValue()->Contents() );
	if (iter != _items.end()) {
		Symbol found = iter->second;
		bool allowed = false;
		switch (found._kind) {
			case Symbol::Type::Var: {
				allowed = true;
				*response = Error::Type::None;
			} break;
			case Symbol::Type::Def: {
				*response = Error::Type::ConstantRedefinition;
				// special case: self inside a function is a def, but it is an
				// implicit parameter, so the user doesn't necessarily notice.
				// we'll give them a more specific error whenever the name of
				// the target symbol happens to be "self", assuming that nobody
				// is going to def a "self" anywhere else.
				if (name->AsValue()->Contents() == "self") {
					*response = Error::Type::SelfConstantRedefinition;
				}
			} break;
			case Symbol::Type::Function: {
				*response = Error::Type::FunctionRedefinition;
			} break;
			case Symbol::Type::Import: {
				*response = Error::Type::ImportRedefinition;
			} break;
			case Symbol::Type::Member: {
				*response = Error::Type::MemberRedefinition;
			} break;
			case Symbol::Type::Undefined: {
				assert( false );
			} break;
		}
		if (allowed) {
			found._value = value;
			iter->second = found;
		}
		return allowed;
	}
	*response = Error::Type::Undefined;
	return false;
}

Symbol SymbolTable::Lookup( Node *name ) const
{
	assert( name && name->IsASymbol() );
	map<string, Symbol>::const_iterator iter =
			_items.find( name->AsValue()->Contents() );
	if (iter != _items.end()) {
		return iter->second;
	}
	return Symbol(Symbol::Type::Undefined, NULL);
}

void SymbolTable::Clear()
{
    _items.clear();
}
