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


#ifndef symbols_h
#define symbols_h

#include "node.h"
#include "token.h"
#include <map>

namespace Semantics {

class Symbol
{
	friend class SymbolTable;
	friend class std::map<std::string, Symbol>;
	public:
		struct Type { enum Enum {
			// Placeholder error type, when lookup fails
			Undefined = 0,
			// Normal read/write symbol
			Var,
			// Read-only symbol definition
			Def,
			// Auto-invoke function reference
			Function,
			// Reference to another module
			Import,
			// Object member: illegal to use directly (use self reference)
			Member
		}; };
		Symbol() : _kind(Symbol::Type::Undefined), _value(NULL) {}
		Symbol( Symbol::Type::Enum kind, Flowgraph::Node *value );
		static Symbol Undefined() { Symbol out; return out; }
		bool IsDefined() const { return _kind != Type::Undefined; }
		Type::Enum Kind() const { return _kind; }
		Flowgraph::Node *Value() const { return _value; }
	private:
		Symbol::Type::Enum _kind;
		Flowgraph::Node *_value;
};

// SymbolTable
//
// We map name tokens to value tokens. Any node is an acceptable key, but the
// intent is that we will use symbol nodes as keys. Symbol nodes thus function
// as intern'ed strings. Tables only search their own items; Radian's context
// semantics require us to wrap containing-scope symbols in environment
// references, so we cannot resole them transparently. Instead, we'll use
// separate symbol tables for local and for captured context symbols.
//
class SymbolTable
{
	public:
		SymbolTable() {}
		explicit SymbolTable( const SymbolTable &src );
        void Insert( Flowgraph::Node *name, Symbol sym );
        bool Update(
				Flowgraph::Node *name,
				Flowgraph::Node *newValue,
				Error::Type::Enum *response=NULL );
        Symbol Lookup( Flowgraph::Node *name ) const;
        void Clear();
    private:
		std::map<std::string, Symbol> _items;
};

} // namespace Semantics

#endif //symbols_h

