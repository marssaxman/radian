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

#ifndef flowgraph_pool_h
#define flowgraph_pool_h

#include <string>
#include <map>
#include "flowgraph/node.h"
#include "flowgraph/value.h"
#include "flowgraph/operation.h"
#include "flowgraph/flowfuncs.h"
#include "flowgraph/intrinsics.h"
#include "lex/sourcelocation.h"

namespace Flowgraph {

// Each program has a semantics pool. Always allocate semantic objects through
// the pool. The pool interns semantic objects; this gives us common
// subexpression elimination, and keeps memory usage to a minimum. Nodes are
// immutable by definition, so we don't bother to mark the pointers const.
class Pool
{
	class Cache
	{
		public:
			Cache() {}
			~Cache();
			Node *Lookup( std::string key ) const;
			void Store( std::string key, Node *value );
		private:
			std::map<std::string, Node*> _items;
	};

	template<class T> class Array
	{
		public:
			Array() {}
			~Array();
			Node *Lookup( unsigned i );
		private:
			std::vector<Node*> _items;
	};

	public:
		Pool( class Delegate &callback, const std::string &filepath );
		~Pool();
		void Validate( bool didReportError );

		Node *Nil() { return &_nil; }
		Node *Self() { return &_self; }
		Node *Function( Node *exp, unsigned int params );
		Node *Function( Node *exp, unsigned int params, std::string name );
		Node *Parameter( unsigned int index );
		Node *Slot( unsigned int index );
		Node *Placeholder( unsigned int index );
		Node *Symbol( std::string value );
		Node *String( std::string value );
		Node *Number( std::string value );
		Node *Number( unsigned int value );
		Node *Float( std::string value );
		Node *Import(
				Node *fileName, Node *sourceDir, const SourceLocation &loc );
		Node *Operation( Operation::Type::Enum type, Node *left, Node *right );
		Node *Inductor( Node *exp );
		Node *Undefined() { return Throw( Sym_Undefined() ); }
		Node *Intrinsic( Intrinsic::ID::Enum id );
		Node *CallN( Node *object, Node *args );
		Node *Call0( Node *object ) { return CallN( object, Nil() ); }
		Node *Call1( Node *object, Node *arg0 );
		Node *Call2( Node *object, Node *arg0, Node *arg1 );
		Node *Call3( Node *object, Node *arg0, Node *arg1, Node *arg2 );
		Node *Call4( Node *object, Node *a0, Node *a1, Node *a2, Node *a3 );
		Node *ArgsAppend( Node *args, Node *val );
		Node *Args1( Node *val ) { return ArgsAppend( Nil(), val ); }
		Node *Args2( Node *arg0, Node *arg1 );
		Node *Args3( Node *arg0, Node *arg1, Node *arg2 );
		Node *Args4( Node *arg0, Node *arg1, Node *arg2, Node *arg3 );
		Node *CaptureN( Node *function, Node *slots );
		Node *Capture1( Node *object, Node *arg0 );
		Node *Capture2( Node *object, Node *arg0, Node *arg1 );
		Node *Loop( Node *start, Node *condition, Node *operation );
		Node *Loop_Sequencer( Node *condition, Node *operation, Node *value );
		Node *Loop_Task( Node *condition, Node *operation, Node *value );
		Node *True();
		Node *False();
		Node *Branch( Node *condition, Node *thenVal, Node *elseVal );
		Node *Not( Node *val );
		Node *TupleAppend( Node *head, Node *tail );
		Node *TupleN( Node *args );
		Node *Tuple1( Node *arg0 );
		Node *Tuple2( Node *arg0, Node *arg1 );
		Node *Tuple3( Node *arg0, Node *arg1, Node *arg2 );
		Node *Tuple4( Node *arg0, Node *arg1, Node *arg2, Node *arg3 );
		Node *SetterSymbol( std::string sym ) { return Symbol( sym + "=" ); }
		Node *SetterSymbol( Node *sym );
		Node *ImportCore();
		Node *Compare( Node *left, Node *right );
		Node *Throw( Node *exp );
		Node *Catch( Node *exp, Node *handler );
		Node *Parallelize( Node *exp );
		Node *IsNotVoid( Node *exp );
		Node *IsNotExceptional( Node *exp );
		Node *MapBlank();
		Node *List( Node *exp );
		Node *Dummy();
		Node *Assert( Node *condition, Node *message );
		Node *Chain( Node *head, Node *tail );

		// The symbol finders provide access to symbols representing names
		// defined by the compiler or names it expects to look up in the
		// language-core module. We can create symbols here which are not
		// accessible from source code by picking reserved words or token which
		// are not identifiers.
		Node *Sym_Add() { return Symbol("add"); }
		Node *Sym_Argv() { return Symbol("argv"); }
		Node *Sym_Assign() { return Symbol("assign"); }
		Node *Sym_Assert() { return Symbol("assert"); }
		Node *Sym_Compare_To() { return Symbol("compare_to"); }
		Node *Sym_Concatenate() { return Symbol("concatenate"); }
		Node *Sym_Core() { return Symbol("core"); }
		Node *Sym_Current() { return Symbol("current"); }
		Node *Sym_Divide() { return Symbol("divide"); }
		Node *Sym_Each() { return Symbol("each"); }
		Node *Sym_Execute() { return Symbol("execute"); }
		Node *Sym_Exponentiate() { return Symbol("exponentiate"); }
		Node *Sym_Filter() { return Symbol("filter"); }
		Node *Sym_Insert() { return Symbol("insert"); }
		Node *Sym_Is_Valid() { return Symbol("is_valid"); }
		Node *Sym_Iterate() { return Symbol("iterate"); }
		Node *Sym_IO() { return Symbol("io"); }
		Node *Sym_Lookup() { return Symbol("lookup"); }
		Node *Sym_Make_Action() { return Symbol("make_action"); }
		Node *Sym_Make_Iterator() { return Symbol("make_iterator"); }
		Node *Sym_Make_Seq_Or_Task() { return Symbol("make_seq_or_task"); }
		Node *Sym_Make_Subsequence() { return Symbol("make_subsequence"); }
		Node *Sym_Make_Subtask() { return Symbol("make_subtask"); }
		Node *Sym_Make_Terminator() { return Symbol("make_terminator"); }
		Node *Sym_Map() { return Symbol("map"); }
		Node *Sym_Modulus() { return Symbol("modulus"); }
		Node *Sym_Multiply() { return Symbol("multiply"); }
		Node *Sym_Next() { return Symbol("next"); }
		Node *Sym_Radian() { return Symbol("radian"); }
		Node *Sym_Result() { return Symbol("result"); }
		Node *Sym_Self() { return Symbol("self"); }
		Node *Sym_ShiftLeft() { return Symbol("shift_left"); }
		Node *Sym_ShiftRight() { return Symbol("shift_right"); }
		Node *Sym_Start() { return Symbol("start"); }
		Node *Sym_Subtract() { return Symbol("subtract"); }
		Node *Sym_Undefined() { return Symbol("undefined"); }
		Node *Sym_Wildcard() { return Symbol("*"); }
		Node *Sym_Yield() { return Symbol("yield"); }

		// Place for the semantic analyzer to store functions or fragments it
		// uses repeatedly
		Node *PadLookup( std::string key ) const;
		void PadStore( std::string key, Node *val );

	protected:
		Node *BinOp( Intrinsic::ID::Enum id, Node *left, Node *right );
		void Taint( const char *condition, const char *file, int line );

	private:
		Value _nil;
		Node *_true;
		Node *_false;
		Node *_not;
		Flowgraph::Self _self;
		Array<Flowgraph::Parameter> _parameters;
		Array<Flowgraph::Slot> _slots;
		Array<Flowgraph::Intrinsic> _intrinsics;
		Array<Flowgraph::Placeholder> _placeholders;
		Cache _functions;
		Cache _strings;
		Cache _numbers;
		Cache _floats;
		Cache _symbols;
		Cache _operations[Operation::Type::COUNT];
		Cache _imports;
		Cache _inductors;
		std::map<std::string, Node*> _pad;
		Delegate &_callback;
		std::string _filePath;
		std::string _privacyID;
		bool _tainted;
		std::string _checkMessage;
};

class Delegate
{
	public:
		virtual ~Delegate() {}
		virtual void PooledFunction( Function *it ) = 0;
		virtual void PooledImport( Import *it, const SourceLocation &loc ) = 0;
};

} // namespace Flowgraph

#endif //pool_h
