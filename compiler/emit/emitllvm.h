// Copyright 2010-2012 Mars Saxman.
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
// Radian. If not, see <http://www.gnu.org/licenses/>.

#ifndef emitllvm_h
#define emitllvm_h

#include "sequence.h"
#include "postorderdfs.h"
#include "linearcode.h"
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>

namespace EmitLLVM {

class Engine
{
	friend class EmitAtom;
	public:
		Engine( Iterator<Flowgraph::Function*> &source );
		virtual ~Engine() {}
		llvm::Module * Run( 
				llvm::LLVMContext &context, 
				std::string name, 
				std::string targetTriple );
	
	protected:
		llvm::PointerType *ObjectType() { return _objectType; }
		llvm::StructType *ObjectStructType()
				{ return _objectStructType; }	
		llvm::PointerType *StringType();
		llvm::IntegerType *Int32Type();
		llvm::IntegerType *BoolType();
		llvm::PointerType *ZoneType() { return _zoneType; }
		llvm::FunctionType * FunctionType() { return _functionType; }
		std::string GlobalName( Flowgraph::Function *node ) const;

		llvm::Module *_module;
		llvm::LLVMContext *_context;

	private:
		friend class Atom;
		
		virtual void EmitMain( Flowgraph::Function *root ) = 0;
		
		std::string GlobalName( const LIC::Addr addr ) const;
		void SetupTypes();
		
		llvm::PointerType *_zoneType;
		llvm::PointerType *_objectType;
		llvm::StructType *_objectStructType;
		llvm::FunctionType *_functionType;
		Iterator<Flowgraph::Function*>& _source;
};

class Program : public Engine
{
	public:
		Program( Iterator<Flowgraph::Function*> &source ) : Engine(source) {}
		
	protected:
		virtual void EmitMain( Flowgraph::Function *root );
};

class Module : public Engine
{
	public:
		Module( std::string ident, Iterator<Flowgraph::Function*> &source ) :
				Engine(source), _ident(ident) {}
		
	protected:
		virtual void EmitMain( Flowgraph::Function *root );
	private:
		std::string _ident;
};

} // namespace EmitLLVM

#endif // emitllvm_h

