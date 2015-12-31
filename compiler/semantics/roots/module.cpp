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


#include "module.h"
using namespace Semantics;
using namespace Flowgraph;

ModuleRoot::ModuleRoot( Pool &pool, Reporter &log, std::string name ):
	Root(pool, log),
	_members(pool),
	_name(name),
	_selfSym(pool.Symbol(name))
{
}

// ModuleRoot::ExitBlock
//
// A module is a parameterless constructor function which returns an object,
// whose contents are the top-level declarations in the module file. It is
// exactly as though the entire file were contained in an "object" block,
// except that the "self" variable takes the file's base name (that is, the
// same name you'd use to import it from another file).
//
Node *ModuleRoot::ExitRoot()
{
	Node *object = _members.Result( *this );
	return _pool.Function( object, 0 );
}

void ModuleRoot::Define(
	Flowgraph::Node *sym,
	Flowgraph::Node *value,
	Symbol::Type::Enum kind,
	const SourceLocation &loc )
{
	if (_members.IsMemberizable( kind )) {
		// Add this item to the member index.
		_members.Define( sym, value, kind );
		// Turn the member into a "member" symbol, so we don't let later code
		// accidentally refer to members without qualification, which would
		// break function calls and var references.
		kind = Symbol::Type::Member;
	}
	// Other than that, things are pretty much normal.
	inherited::Define( sym, value, kind, loc );
}

void ModuleRoot::Assign(
		Flowgraph::Node *sym,
		Flowgraph::Node *value,
		const SourceLocation &loc )
{
	// Modules are an object scope, so they can contain only definitions - no
	// statements which alter the definition of an existing symbol.
	ReportError( Error::Type::ModuleMemberRedefinition, loc );
}

// ModuleRoot::EnableBuiltins
//
// The lowest-level parts of the Radian support library are implemented in C,
// since Radian itself doesn't offer particularly good abstractions for things
// like allocators or garbage collectors. Other parts are implemented in Radian
// code, located in a Radian-specific directory shared by all programs. The
// Radian-implemented parts of the support library need a private interface to
// the C-implemented parts of the library, so we don't have to expose the low-
// level details as a supported part of the standard library interface. One
// option might be to just add some hard-to-guess identifiers to the language
// as builtins, then refrain from documenting them, but this approach is a bit
// safer: if and only if the current module is part of the Radian library, we
// will enable some extra "builtins" which link against those C-implemented
// runtime support features. This way you cannot even accidentally refer to
// these features from any Radian code outside the standard library.
//
void ModuleRoot::EnableBuiltins(void)
{
	BuiltinDef( "map_blank", Intrinsic::ID::Map_Blank );
	BuiltinDef( "list_blank", Intrinsic::ID::List_Blank );
	BuiltinFunction( "char_from_int", Intrinsic::ID::Char_From_Int );
	BuiltinFunction( "ffi_load_external", Intrinsic::ID::FFI_Load_External );
	BuiltinFunction(
			"ffi_describe_function", Intrinsic::ID::FFI_Describe_Function );
	BuiltinFunction( "ffi_call", Intrinsic::ID::FFI_Call );
	BuiltinFunction( "read_bytes", Intrinsic::ID::Read_File );
	BuiltinFunction( "write_bytes", Intrinsic::ID::Write_File );
	BuiltinFunction( "sin", Intrinsic::ID::Sin );
	BuiltinFunction( "cos", Intrinsic::ID::Cos );
	BuiltinFunction( "tan", Intrinsic::ID::Tan );
	BuiltinFunction( "asin", Intrinsic::ID::Asin );
	BuiltinFunction( "acos", Intrinsic::ID::Acos );
	BuiltinFunction( "atan", Intrinsic::ID::Atan );
	BuiltinFunction( "atan2", Intrinsic::ID::Atan2 );
	BuiltinFunction( "sinh", Intrinsic::ID::Sinh );
	BuiltinFunction( "cosh", Intrinsic::ID::Cosh );
	BuiltinFunction( "tanh", Intrinsic::ID::Tanh );
	BuiltinFunction( "asinh", Intrinsic::ID::Asinh );
	BuiltinFunction( "acosh", Intrinsic::ID::Acosh );
	BuiltinFunction( "atanh", Intrinsic::ID::Atanh );
	BuiltinFunction( "to_float", Intrinsic::ID::To_Float );
	BuiltinFunction( "floor_float", Intrinsic::ID::Floor_Float );
	BuiltinFunction( "ceiling_float", Intrinsic::ID::Ceiling_Float );
	BuiltinFunction( "truncate_float", Intrinsic::ID::Truncate_Float );
}

void ModuleRoot::BuiltinFunction( std::string name, Intrinsic::ID::Enum id )
{
	Builtin( name, id, Symbol::Type::Function );
}

void ModuleRoot::BuiltinDef( std::string name, Intrinsic::ID::Enum id )
{
	Builtin( name, id, Symbol::Type::Def );
}

void ModuleRoot::Builtin(
		std::string name, Intrinsic::ID::Enum id, Symbol::Type::Enum type )
{
	SourceLocation nowhere;
	inherited::Define(
			_pool.Symbol( "_builtin_" + name ),
			_pool.Intrinsic( id ),
			type,
			nowhere );
}
