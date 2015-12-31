// Copyright 2011-2014 Mars Saxman.
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

#include "llvmjit.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Linker.h>
#include <assert.h>

extern "C" {
#include "../../runtime/libradian.h"
}

/**
 * Links a bunch of job results together.
 *
 * @param dest The Module to link everything into.
 * @param modules The modules to link in. This vector will be emptied and all
 *                of the Modules destroyed.
 */
void LLVMJIT::LinkModules(
		llvm::Module *dest, std::vector<llvm::Module *> &modules)
{
	for (auto module: modules) {
		assert(&dest->getContext() == &module->getContext());
		assert(!llvm::Linker::LinkModules(
				dest, module, llvm::Linker::DestroySource, NULL));
	}
	modules.clear();
}

/**
 * Executes the given Module's main function in a JIT.
 *
 * @param code The code to execute.
 * @param argv The arguments to pass to main.
 * @result The status code returned from main.
 */
int LLVMJIT::Run(llvm::Module *code, const std::vector<std::string> &argv)
{
	LLVMLinkInJIT();
	llvm::InitializeNativeTarget();
	// GVsWithCode is weird. I don't know why we need to disable it, but if we
	// don't, LLVM sometimes crashes while allocating space for global vars,
	// because it fails to make the allocation and then attempts to zero out
	// the nonexistent buffer it didn't allocate.
	llvm::ExecutionEngine *jit = llvm::ExecutionEngine::createJIT(
			code,		// M
			NULL,	// ErrorStr
			NULL,	// JMM
			llvm::CodeGenOpt::None,
			false	// GVsWithCode
			 );
	jit->DisableSymbolSearching(true);

	// Support functions invoked by code generated while translating LIC into
	// LLVM IR - these use C function semantics
	#define REGISTER_BUILTIN(x) { llvm::Function *_fn = \
			code->getFunction( #x ); \
			if (_fn) jit->addGlobalMapping( _fn, (void *)x ); }
	REGISTER_BUILTIN( init_runtime );
	REGISTER_BUILTIN( root_zone );
	REGISTER_BUILTIN( Args );
	REGISTER_BUILTIN( RunIO );
	REGISTER_BUILTIN( alloc_object );
	REGISTER_BUILTIN( alloc_buffer );
	REGISTER_BUILTIN( clone_buffer );
	REGISTER_BUILTIN( StringLiteral );
	REGISTER_BUILTIN( SymbolLiteral );
	REGISTER_BUILTIN( NumberLiteral );
	REGISTER_BUILTIN( ThrowArgCountFail );
	REGISTER_BUILTIN( IsAnException );
	REGISTER_BUILTIN( NumberFromDouble );
	REGISTER_BUILTIN( Throw );
	REGISTER_BUILTIN( BoolFromBoolean );
	#undef REGISTER_BUILTIN

	// Functions invoked by code generated in the Radian semantics engine -
	// these are the targets of LIC call or capture operations and use Radian
	// closure semantics
	#define REGISTER_BUILTIN(x) { llvm::GlobalVariable *_fn = \
			code->getGlobalVariable( #x ); \
			if (_fn) jit->addGlobalMapping( _fn, (void *)&x ); }
	REGISTER_BUILTIN( True_returner );
	REGISTER_BUILTIN( False_returner );
	REGISTER_BUILTIN( throw_exception );
	REGISTER_BUILTIN( catch_exception );
	REGISTER_BUILTIN( is_not_exceptional );
	REGISTER_BUILTIN( is_not_void );
	REGISTER_BUILTIN( parallelize );
	REGISTER_BUILTIN( make_tuple );
	REGISTER_BUILTIN( map_blank );
	REGISTER_BUILTIN( list );
	REGISTER_BUILTIN( list_empty );
	REGISTER_BUILTIN( loop_sequencer );
	REGISTER_BUILTIN( loop_task );
	REGISTER_BUILTIN( char_from_int );
	REGISTER_BUILTIN( FFI_Load_External );
	REGISTER_BUILTIN( FFI_Describe_Function );
	REGISTER_BUILTIN( FFI_Call );
	REGISTER_BUILTIN( Read_File );
	REGISTER_BUILTIN( Write_File );
	REGISTER_BUILTIN( debug_trace );
	REGISTER_BUILTIN( math_sin );
	REGISTER_BUILTIN( math_cos );
	REGISTER_BUILTIN( math_tan );
	REGISTER_BUILTIN( math_asin );
	REGISTER_BUILTIN( math_acos );
	REGISTER_BUILTIN( math_atan );
	REGISTER_BUILTIN( math_atan2 );
	REGISTER_BUILTIN( math_sinh );
	REGISTER_BUILTIN( math_cosh );
	REGISTER_BUILTIN( math_tanh );
	REGISTER_BUILTIN( math_asinh );
	REGISTER_BUILTIN( math_acosh );
	REGISTER_BUILTIN( math_atanh );
	REGISTER_BUILTIN( to_float );
	REGISTER_BUILTIN( floor_float );
	REGISTER_BUILTIN( ceiling_float );
	REGISTER_BUILTIN( truncate_float );
	#undef REGISTER_BUILTIN

	auto main = code->getFunction( "main" );
	int result = jit->runFunctionAsMain( main, argv, NULL );

	// Remove the module so that the JIT doesn't destroy it.
	jit->removeModule(code);
	delete jit;

	return result;
}
