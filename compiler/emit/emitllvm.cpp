// Copyright 2010-2013 Mars Saxman.
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

#include <string>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Support/Host.h>
#include "emitllvm.h"
#include "llvmatom.h"
#include "flowgraph.h"
#include "numtostr.h"
#include "llvmutils.h"

using namespace std;
using namespace Flowgraph;
using namespace LIC;
using namespace EmitLLVM;

Engine::Engine( Iterator<Flowgraph::Function*> &source ) :
	_module(NULL),
	_context(NULL),
	_zoneType(NULL),
	_objectType(NULL),
	_objectStructType(NULL),
	_functionType(NULL),
	_source(source)
{
}

llvm::Module * Engine::Run(
		llvm::LLVMContext &context, string name, string targetTriple )
{
	if (targetTriple == "") targetTriple = llvm::sys::getDefaultTargetTriple();
	
	_context = &context;
	_module = new llvm::Module( name, *_context );
	_module->setTargetTriple( targetTriple );
	_module->setDataLayout( DataLayoutForTriple( targetTriple ) );
	SetupTypes();
	
	// Bah. Apparently the last node codegenned is magically our main function.
	Node *root = NULL;
	while (_source.Next()) {
		EmitLLVM::Atom atom( _source.Current(), *this );
		atom.Run();
		root = _source.Current();
	}
	
	assert( root && root->IsAFunction() );
	EmitMain( root->AsFunction() );
	return _module;
}

void Engine::SetupTypes()
{
	assert(!_zoneType && !_objectType && !_objectStructType && !_functionType);
	llvm::LLVMContext &context = _module->getContext();
	
	// typedef const struct zone *zone_t;
	llvm::StructType *opaqueZoneTy =
			llvm::StructType::create( context, "zone" );
	llvm::PointerType *zoneTy = opaqueZoneTy->getPointerTo();
	
	// typedef const struct closure *object_t;
	llvm::StructType *closureTy =
			llvm::StructType::create( context, "closure" );
	llvm::PointerType *objectTy = closureTy->getPointerTo();
	
	// typedef object_t (*function_t)(PREFUNC, ...);
	// Must update this list every time you redefine PREFUNC.
	llvm::Type *funcTyArgs[3] =
			{zoneTy, objectTy, llvm::Type::getInt32Ty( context )};
	
	llvm::FunctionType *functionTy = 
		llvm::FunctionType::get( objectTy, funcTyArgs, true ); 
		
	// struct closure {
	//   function_t function;
	//   object_t slots[0];
	// };
	llvm::Type *closureFields[2];
	closureFields[0] = functionTy->getPointerTo();
	closureFields[1] = llvm::ArrayType::get( objectTy, 0 );
	closureTy->setBody( closureFields );	

	_zoneType = zoneTy;
	_objectStructType = closureTy;
	_objectType = objectTy;
	_functionType = functionTy;
}

llvm::PointerType *Engine::StringType()
{
	return llvm::Type::getInt8PtrTy( _module->getContext() );
}

llvm::IntegerType *Engine::Int32Type()
{
	return llvm::Type::getInt32Ty( _module->getContext() );
}

llvm::IntegerType *Engine::BoolType()
{
	return llvm::Type::getInt1Ty( _module->getContext() );
}

string Engine::GlobalName( Function *node ) const
{
	// If you change this, be sure to update Atom::Load()!
	return "noslot_" + node->Name();
}

string Engine::GlobalName( const Addr addr ) const
{
	return "noslot_" + addr.Link();
}

void Program::EmitMain( Function *root )
{
	assert( root );
	
	llvm::IRBuilder<> builder( *_context );
	
	llvm::Value *genericMain = _module->getOrInsertFunction( 
		"main", 
		Int32Type(),
		Int32Type(), // int argc
		StringType()->getPointerTo(), // char **argv
		NULL );
	llvm::Function *main = llvm::cast<llvm::Function>( genericMain );
    llvm::Function::arg_iterator args = main->arg_begin();
	llvm::Value *argc = args++;
	argc->setName("argc");
	llvm::Value *argv = args++;
	argv->setName("argv");
    
	llvm::BasicBlock *entry = 
		llvm::BasicBlock::Create( _module->getContext(), "entry", main );
	builder.SetInsertPoint( entry );
	
	// Initialize the Radian runtime
	llvm::Value *initFunc = _module->getOrInsertFunction( 
		"init_runtime", 
		ZoneType(), 
		NULL );
	llvm::Value *rootZone = builder.CreateCall( initFunc );
	
	// Pack up argc and argv into a sequence the Radian code can iterate over
	llvm::Value *argsFunc = _module->getOrInsertFunction(
		"Args",
		ObjectType(), 
		ZoneType(),
		Int32Type(), 
		StringType()->getPointerTo(), 
		NULL );
	llvm::Value *argSequence = 
		builder.CreateCall3( argsFunc, rootZone, argc, argv );
	
	// Get a reference to the Radian root function closure
	llvm::Value *rootFunc = 
		_module->getOrInsertGlobal( GlobalName( root ), ObjectStructType() );
	
	// Call the RunIO function which will drive the I/O process using the
	// result of the root function.
	llvm::Value *runIoFunc = _module->getOrInsertFunction( 
		"RunIO", 
		Int32Type(), 
		ObjectType(), 
		ObjectType(),
		NULL );
	llvm::Value *result = builder.CreateCall2( 
		runIoFunc, 
		rootFunc, 
		argSequence );

	builder.CreateRet( result );
}

void Module::EmitMain( Function *root )
{
	// A module is basically a singleton object. Module users expect to be able
	// to call some external function which returns the module object. They can
	// then do whatever they want with it, which generally means  invoking some
	// of its members. 
	//
	// The root function is a factory that generates an instance of the module
	// object. This could actually be the linked module function itself, but
	// there's no reason to keep generating the same object over and over, so
	// we will instead have a module entrypoint function that caches the module
	// object in a static var. The entrypoint function should be the only
	// non-static function in the output file.
	assert( root );
	llvm::IRBuilder<> builder( *_context );

	// This is the function we need to call to create the object and the data
	// we allocated earlier for an object with no slots. These should have been
	// created earler when we ran emitatom over it.
	llvm::Function *objectFunc = 
			_module->getFunction( root->AsFunction()->Name() );
	llvm::GlobalVariable *objectData = 
			_module->getNamedGlobal( GlobalName( root ) );
	assert( objectFunc && objectData );
	
	// We'll store the created module object in this global variable.
	llvm::Constant *nullObject = llvm::Constant::getNullValue( ObjectType() );
	llvm::GlobalVariable *moduleCache = new llvm::GlobalVariable( 
			*_module, 
			ObjectType(), 
			false, 
			llvm::GlobalVariable::InternalLinkage, 
			nullObject, 
			"s_module_" + _ident );
	
	// Now make the module function and a bunch of basic blocks that we'll need.
	llvm::Function *moduleFunc = llvm::Function::Create( 
			GenerateFunctionType( ObjectType(), ZoneType(), NULL ), 
			llvm::Function::ExternalLinkage, 
			"module_" + _ident, _module );
	llvm::BasicBlock *entry = llvm::BasicBlock::Create( 
			*_context, "entry", moduleFunc );
	llvm::BasicBlock *makeObject = llvm::BasicBlock::Create( 
			*_context, "makeObject", moduleFunc );
	llvm::BasicBlock *exit = llvm::BasicBlock::Create( 
			*_context, "exit", moduleFunc );
	
	// Check to see we've already created this module object
	builder.SetInsertPoint( entry );
	llvm::Value *cmpResult = builder.CreateICmpEQ( 
			builder.CreateLoad( moduleCache ), 
			nullObject );
	builder.CreateCondBr( cmpResult , makeObject, exit );
	
	// The module is NULL, so we're going to need to invoke its function and
	// stash the result away.
	builder.SetInsertPoint( makeObject );
	// Look up the root zone, which is where we will create this object
	llvm::Value *zoneFunc = _module->getOrInsertFunction( 
		"root_zone", 
		ZoneType(), 
		NULL );
	llvm::Value *rootZone = builder.CreateCall( zoneFunc );
	builder.CreateStore( 
			builder.CreateCall3( 
					objectFunc, 
					rootZone,
					objectData, // self
					builder.getInt32( 0 ) ),	// argc
			moduleCache );
	builder.CreateBr( exit );
	
	// Now return whatever we just created (or already existed)
	builder.SetInsertPoint( exit );
	builder.CreateRet( builder.CreateLoad( moduleCache ) );
}
