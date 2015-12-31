// Copyright 2010-2014 Mars Saxman.
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

#ifndef llvmatom_h
#define llvmatom_h

#include <stack>
#include "emitllvm.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include "linearcode.h"
#include "licaddr.h"
#include "linearizer.h"

namespace EmitLLVM {

class Atom
{
	public:
		Atom( Flowgraph::Function *root, Engine &host );
		void Run();

	protected:
		llvm::Value *ProcessSelf( const LIC::Op &it );
		llvm::Value *ProcessNumberLiteral( const LIC::Op &it );
		llvm::Value *ProcessFloatLiteral( const LIC::Op &it );
		llvm::Value *ProcessStringLiteral( const LIC::Op &it );
		llvm::Value *ProcessSymbolLiteral( const LIC::Op &it );
		llvm::Value *ProcessParameter( const LIC::Op &it );
		llvm::Value *ProcessSlot( const LIC::Op &it );
		llvm::Value *ProcessImport( const LIC::Op &it );
		llvm::Value *ProcessRepeat( const LIC::Op &it );
		llvm::Value *ProcessLoopWhile( const LIC::Op &it );
		llvm::Value *ProcessAssert( const LIC::Op &it );
		llvm::Value *ProcessChain( const LIC::Op &it );
		llvm::Value *ProcessCall( const LIC::Op &it );
		llvm::Value *ProcessCapture( const LIC::Op &it );
		void Process( const LIC::Op& it );

		void Store(
				llvm::Value *value, LIC::Addr regnum, std::string name = "" );
		llvm::Value * Load( LIC::Addr addr );
		llvm::Value * GetParameter( int index );
		llvm::Value * GetImplicitSelf();
		llvm::Value * GetImplicitZone();
		llvm::Value * GetImplicitArgc();
		void CreateGlobalSingleton();
		llvm::BasicBlock *CreateBasicBlock( std::string name );
		llvm::Function *CreateFunction( unsigned arity, std::string name );
		std::string VarName( const LIC::Op &it );
        llvm::BasicBlock *CheckArgumentCount( std::string name );

	private:
		PostOrderDFS _source;
		Flowgraph::Function *_root;
		Engine& _host;
		llvm::Module *_module;
		llvm::IRBuilder<> _builder;
		llvm::Function *_function;
		unsigned _blockNum;
		unsigned _slotCount;
		std::map<int, llvm::Value *> _registers;
		struct LoopState
		{
			llvm::BasicBlock *begin;
			llvm::BasicBlock *end;
			llvm::AllocaInst *valuePtr;
		};
		std::stack<LoopState> _loops;
};

} // namespace EmitLLVM

#endif //llvmatom_h
