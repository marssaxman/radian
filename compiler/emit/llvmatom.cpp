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
// Radian.  If not, see <http://www.gnu.org/licenses/>.

#include "llvmatom.h"
#include "numtostr.h"
#include "unicode.h"
#include <algorithm>

using namespace std;
using namespace Flowgraph;
using namespace LIC;
using namespace EmitLLVM;


static const int kImplicitParameterCount = 3;
static const int kImplicitZoneParameterIndex = 0;
static const int kImplicitSelfParameterIndex = 1;
static const int kImplicitArgcParameterIndex = 2;
static string kIntrinsicArgcFailFunctionName = "ThrowArgCountFail";
static string kIntrinsicExceptionCheckFunctionName = "IsAnException";
static string kIntrinsicThrowFunctionName = "Throw";
static string kIntrinsicBooleanToBoolFunctionName = "BoolFromBoolean";

Atom::Atom( Flowgraph::Function *root, Engine &host ) :
	_source(root->Exp()),
	_root(root),
	_host(host),
	_module(host._module),
	_builder(*host._context),
	_function(NULL),
	_blockNum(0),
	_slotCount(0)
{
}

// Atom::Run
//
// Main process driving LLVM: we have been configured with an input function in
// LIC form and must drive LLVM to produce equivalent machine code.
// 
void Atom::Run()
{
	assert(_registers.size() == 0);
	_function = CreateFunction( _root->Arity(), _root->Name() );

	// Validate the arguments. Make sure we can safely perform the computation
	// described by the body of this function.
	llvm::BasicBlock *body = CheckArgumentCount( "body" );

	// Generate a main block which contains the body of the function. 
	// Linearize the source function and generate code into this main block.
	_builder.SetInsertPoint( body );
	Linearizer l( _source );
	while (l.Next()) {
		Op *op = l.Current();
		assert( op );
		Process( *op );
	}
	llvm::BasicBlock *exit = CreateBasicBlock( "exit" );
	_builder.CreateBr( exit );
	
	// The exit block simply returns the function's result value.
	_builder.SetInsertPoint( exit );	
	_builder.CreateRet( Load( l.Result() ) );
	
	// If this is a simple function (one with no slots), we will create a 
	// global object, representing an invokable instance of the function.
	if (_slotCount == 0) {
		CreateGlobalSingleton();
	}
}

llvm::BasicBlock *Atom::CheckArgumentCount( std::string nextBlockName )
{
	// Generate a prolog that checks the argument count. The caller will tell
	// us how many arguments it supplied, not including the zone and the argc
	// itself; we must make sure this matches the number of args expected.
	llvm::BasicBlock *entry = CreateBasicBlock( "entry" );
	_builder.SetInsertPoint( entry );
	llvm::Value *actualArgc = GetImplicitArgc();
	llvm::Value *expectedArgc = _builder.getInt32( _root->Arity() );
	llvm::Value *argcEqual = _builder.CreateICmpEQ( actualArgc, expectedArgc );
	llvm::BasicBlock *body = CreateBasicBlock( nextBlockName );
	llvm::BasicBlock *argcfail = CreateBasicBlock( "argcfail" );
	_builder.CreateCondBr( argcEqual, body, argcfail );
	
	// If the argument count is wrong, we will invoke an intrinsic which raises
	// and returns an appropriate exception. 
	_builder.SetInsertPoint( argcfail );
	llvm::Value *failfunc = _module->getOrInsertFunction( 
			kIntrinsicArgcFailFunctionName,
			_host.ObjectType(),         // return type
			_host.ZoneType(),           // alloc zone parameter
			_builder.getInt8PtrTy(),    // name of the function
			_builder.getInt32Ty(),      // expected number of args
			_builder.getInt32Ty(),      // actual number of args
			NULL );
	llvm::Value *funcname = _builder.CreateGlobalStringPtr( _root->Name() );
	llvm::Value *exception = _builder.CreateCall4(
			failfunc, GetImplicitZone(), funcname, expectedArgc, actualArgc );
	_builder.CreateRet( exception );
	return body;
}

llvm::BasicBlock *Atom::CreateBasicBlock( std::string name )
{
	return llvm::BasicBlock::Create( _module->getContext(), name, _function );
}

llvm::Function *Atom::CreateFunction( unsigned arity, std::string name )
{
	// Make an array of types for the arguments this function will accept.
	std::vector<llvm::Type *> argTypes;
	argTypes.push_back( _host.ZoneType() );	// allocation zone
	argTypes.push_back( _host.ObjectType() ); // implicit self
	argTypes.push_back( _builder.getInt32Ty() ); // argument count
	for (unsigned i = 0; i < arity; i++) {
		argTypes.push_back( _host.ObjectType() );
	}

	// Create a type reference for this function using the argument list we
	// have just set up, then create the function.
	llvm::FunctionType *funcType =
			llvm::FunctionType::get( _host.ObjectType(), argTypes, false );
	llvm::Value *func = _host._module->getOrInsertFunction( name, funcType );

	// All of our functions have private linkage, because we have our own
	// mechanism for exporting the contents of a module.
	llvm::Function *result = llvm::cast<llvm::Function>( func );
	result->setLinkage( llvm::GlobalValue::PrivateLinkage );
	return result;
}


void Atom::CreateGlobalSingleton(void)
{
	assert( _root && 0 == _slotCount );
	string globalName = _host.GlobalName( _root );
	llvm::Constant *constant = _module->getOrInsertGlobal( 
			globalName, _host.ObjectStructType() );
	llvm::GlobalVariable *globalObject = 
			llvm::cast<llvm::GlobalVariable>( constant );
	globalObject->setLinkage( llvm::GlobalValue::PrivateLinkage );
	
	// Sadly, all of the types have to match *exactly* and we need to 
	// specify even the zero length array.
	vector<llvm::Constant *> fields;
	
	llvm::Type *funcTy = _host.FunctionType()->getPointerTo();
	llvm::Constant *funcPtr = llvm::ConstantExpr::getCast( 
			llvm::Instruction::BitCast, _function, funcTy );
	fields.push_back( funcPtr );
	
	llvm::Type *arrayType = 
			llvm::ArrayType::get( _host.ObjectType(), 0 );
	fields.push_back( llvm::ConstantAggregateZero::get( arrayType ) );
	
	llvm::Constant *initializer = 
			llvm::ConstantStruct::get( _host.ObjectStructType(), fields );
	globalObject->setInitializer( initializer );
}

void Atom::Store( llvm::Value *value, LIC::Addr addr, std::string name )
{
	assert(value);
	assert(_registers[addr.Register()] == NULL);
	assert(value->getType() == _host.ObjectType());
	
	std::string fullName = "reg" + numtostr_dec(addr.Register());
	if(name != "") {
		fullName += "_" + name;
	}
	
	value->setName(fullName);
	_registers[addr.Register()] = value;
}

llvm::Value * Atom::Load( LIC::Addr addr )
{
	switch(addr.Type()) {
	case Addr::Type::Intrinsic: {
		return _module->getOrInsertGlobal( 
				addr.Intrinsic(), _host.ObjectStructType() );
	} break;
	case Addr::Type::Void: {
		return llvm::Constant::getNullValue(_host.ObjectType());
	} break;
	case Addr::Type::Register: {
		llvm::Value *result = _registers[addr.Register()];
		assert(result);
		return result;
	} break;
	case Addr::Type::Link: {
		return _module->getOrInsertGlobal( 
				_host.GlobalName( addr ), _host.ObjectStructType() );
	} break;
	case Addr::Type::Data:
		// The only place we should get a Data address is when invoking one of
		// the literal functions (Number, String, Error, Symbol, Import).
	default:
		assert(0);
		return NULL;
	}
}

llvm::Value *Atom::ProcessSelf( const Op& it )
{
	assert( it.Code() == Op::Code::Self );
    return GetImplicitSelf();
}

llvm::Value *Atom::ProcessNumberLiteral( const Op& it )
{
	assert( it.Code() == Op::Code::NumberLiteral );
	llvm::Value *func = _module->getOrInsertFunction( 
			it.Name(), 
			_host.ObjectType(),		// return type
			_host.ZoneType(),		// alloc zone parameter
			_builder.getInt8PtrTy(), // data pointer
			_builder.getInt32Ty(), // data size
			NULL );
	llvm::Value *zone = GetImplicitZone();
	llvm::Value *literal = 
		_builder.CreateGlobalStringPtr( it.Value().Data().c_str() );
	llvm::Value *length =
		_builder.getInt32( it.Value().Data().size() );
	return _builder.CreateCall3( func, zone, literal, length );
}

llvm::Value *Atom::ProcessFloatLiteral( const Op& it )
{
	assert( it.Code() == Op::Code::FloatLiteral );
	llvm::Type *doubleType = _builder.getDoubleTy();
	llvm::Value *func = _module->getOrInsertFunction( 
			it.Name(), 
			_host.ObjectType(),		// return type
			_host.ZoneType(),		// alloc zone parameter
			doubleType, // data value
			NULL );
	llvm::Value *zone = GetImplicitZone();
	std::string value = it.Value().Data();
	// We expect this to be a Radian float literal, which is a series of
	// decimal digits, including one decimal point, followed by a letter F,
	// which is either upper or lower case. The format ConstantFP expects is
	// almost exactly like this, minus the suffix.
	assert( value.size() > 1 );
	size_t value_tail = value.size() - 1;
	assert( value[value_tail] == 'f' || value[value_tail] == 'F' );
	value.resize( value_tail );
	llvm::Value *literal = llvm::ConstantFP::get( doubleType, value );
	return _builder.CreateCall2( func, zone, literal );
}

llvm::Value *Atom::ProcessStringLiteral( const Op& it )
{
	assert( it.Code() == Op::Code::StringLiteral );
	llvm::Value *func = _module->getOrInsertFunction(
			it.Name(), 
			_host.ObjectType(),		// return type
			_host.ZoneType(),		// alloc zone parameter
			_builder.getInt8PtrTy(), // data pointer
			_builder.getInt32Ty(), // data size
			NULL );
	llvm::Value *zone = GetImplicitZone();
	llvm::Value *literal = 
		_builder.CreateGlobalStringPtr( it.Value().Data().c_str() );
	llvm::Value *length =
		_builder.getInt32( it.Value().Data().size() );
	return _builder.CreateCall3( func, zone, literal, length );
}

llvm::Value *Atom::ProcessSymbolLiteral( const Op& it )
{
	assert( it.Code() == Op::Code::SymbolLiteral );
	llvm::Value *func = _module->getOrInsertFunction( 
			it.Name(), 
			_host.ObjectType(),		// return type
			_host.ZoneType(),		// alloc zone parameter
			_builder.getInt8PtrTy(), // data pointer
			NULL );
	llvm::Value *zone = GetImplicitZone();
	llvm::Value *literal = 
		_builder.CreateGlobalStringPtr( it.Value().Data().c_str() );
	return _builder.CreateCall2( func, zone, literal );
}

llvm::Value *Atom::ProcessParameter( const Op& it )
{
	assert( it.Code() == Op::Code::Parameter );
	int index = it.Value().Index();	
	// Adjust the index for the argument count and the implicit self.
	return GetParameter( index + kImplicitParameterCount );
}

llvm::Value *Atom::ProcessSlot( const Op& it )
{
	assert( it.Code() == Op::Code::Slot );
	unsigned index = it.Value().Index();
	_slotCount = max(_slotCount, index + 1);
	
	// GEP is such a fun instruction. Basically the next few lines are
	// equivalent to the following bit of C:
	//   p = &self[0].slots;
	//   p = p + index;
	//   p = *p;
	llvm::Value *slots = _builder.CreateStructGEP(GetImplicitSelf(), 1);
	llvm::Value *slot = _builder.CreateConstInBoundsGEP2_32(slots, 0, index);
	return _builder.CreateLoad(slot);
}

llvm::Value *Atom::ProcessImport( const Op& it )
{
	assert( it.Code() == Op::Code::Import );
	// Import is simply a reference to a module object that happens
	// to live in another file.
	llvm::Value *func = _module->getOrInsertFunction( 
			"module_" + it.Value().Data(), 
			_host.ObjectType(), 
			_host.ZoneType(),
			NULL);
	llvm::Value *zone = GetImplicitZone();
	return _builder.CreateCall( func, zone );
}

llvm::Value *Atom::ProcessRepeat( const Op& it )
{
	assert( it.Code() == Op::Code::Repeat );
	// Complete a loop begun with LoopWhile.
	// Jump back to the beginning of the loop and let it run through the
	// condition logic again. At some point someone will eventually branch to
	// the end block, which we will populate.
	LoopState state = _loops.top();
	_loops.pop();
	llvm::Value *outResult = Load( it.Value() );
	_builder.CreateStore( outResult, state.valuePtr );
	_builder.CreateBr( state.begin );
	_builder.SetInsertPoint( state.end );
	return _builder.CreateLoad( state.valuePtr );
}

llvm::Value *Atom::ProcessLoopWhile( const Op& it )
{
	assert( it.Code() == Op::Code::LoopWhile );
	vector<llvm::Value *> args;

	// Create the loop variable and assign its initial value.
	string loopValName = "loop_value_" + numtostr_dec( _blockNum );
	llvm::AllocaInst *loopValPtr = _builder.CreateAlloca(
			_host.ObjectType(), 0, loopValName );
	llvm::Value *initialValue = Load( it.Left() );
	llvm::Value *conditionFunc = Load(it.Right() );
	_builder.CreateStore( initialValue, loopValPtr );
	
	// Establish the loop re-entry point. The Repeat op will jump here.
	string loopBeginName = "loop_begin_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *begin = CreateBasicBlock( loopBeginName );
	_builder.CreateBr( begin );
	_builder.SetInsertPoint( begin );
	// Create another basic block, to be populated later, which is the loop
	// exit point.
	string loopEndName = "loop_end_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *end = CreateBasicBlock( loopEndName );
	// Save the begin and end block pointers and the loop value pointer for
	// later use by the repeat operation.
	LoopState state = {begin, end, loopValPtr};
	_loops.push( state );

	// Get the current loop value. If it is an exception, end the loop; else
	// fall through to the condition test.
	llvm::Value *loopVal = _builder.CreateLoad( loopValPtr, loopValName );
	llvm::Value *exTestFunc = _module->getOrInsertFunction(
			kIntrinsicExceptionCheckFunctionName,
			_host.BoolType(),		// return type
			_host.ObjectType(),			// input parameter
			NULL );
	args.clear();
	args.push_back( loopVal );
	llvm::Value *isException = _builder.CreateCall( exTestFunc, args );
	string loopConditionName = "loop_condition_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *callCondition = CreateBasicBlock( loopConditionName );
	_builder.CreateCondBr( isException, end, callCondition );
	_builder.SetInsertPoint( callCondition );
	
	// Call the condition function, passing in the current loop value, to get
	// a condition value which will tell us whether we should keep looping.
	llvm::Value *target = conditionFunc;
	llvm::Value *function =
			_builder.CreateLoad( _builder.CreateStructGEP( target, 0 ) );
	args.clear();
	args.push_back( GetImplicitZone() );
	args.push_back( target );
	args.push_back( _builder.getInt32( 1 ) );
	args.push_back( loopVal );
	llvm::Value *condition = _builder.CreateCall( function, args );
	
	// Find out if the condition is an exception. If so, we must write it out
	// as the loop output value, then jump to the exit point.
	args.clear();
	args.push_back( condition );
	isException = _builder.CreateCall( exTestFunc, args );
	string loopTestName = "loop_test_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *loopTest = CreateBasicBlock( loopTestName );
	string loopAbortName = "loop_abort_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *loopAbort = CreateBasicBlock( loopAbortName );
	_builder.CreateCondBr( isException, loopAbort, loopTest );
	_builder.SetInsertPoint( loopAbort );
	_builder.CreateStore( condition, loopValPtr );
	_builder.CreateBr( end );
	_builder.SetInsertPoint( loopTest );

	// Turn this condition object into a boolean we can actually use. That
	// means we have to call it, passing in two parameters, and see whether it
	// returns the first (true) or second (false). 	We already know it's not
	// an exception so there is no reason it should return any bogus value,
	// but we will still fail safe by aborting if we don't get the value we
	// expect.
	target = condition;
	function = _builder.CreateLoad( _builder.CreateStructGEP( target, 0 ) );
	args.clear();
	args.push_back( GetImplicitZone() );
	args.push_back( target );
	args.push_back( _builder.getInt32( 2 ) );
	args.push_back( target ); // true value
	args.push_back( llvm::Constant::getNullValue(_host.ObjectType()) ); // false value
	llvm::Value *condResult = _builder.CreateCall( function, args );
	llvm::Value *should_continue = _builder.CreateICmpEQ( target, condResult );
	string loopBodyName = "loop_body_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *loopBody = CreateBasicBlock( loopBodyName );
	_builder.CreateCondBr( should_continue, loopBody, end );
	_builder.SetInsertPoint( loopBody );

	// The result value is the current loop value.
    return _builder.CreateLoad( loopValPtr, loopValName );
}

llvm::Value *Atom::ProcessAssert( const Op& it )
{
	assert( it.Code() == Op::Code::Assert );
	vector<llvm::Value *> args;
	// If the condition is true, we return it. If the condition is false, we
	// throw the message as an exception and return that.
	llvm::Value *condition = Load( it.Left() );
	llvm::Value *message = Load( it.Right() );

	// Create the output variable and assign its initial value, which will be
	// the "true" condition.
	string valName = "assert_" + numtostr_dec( _blockNum );
	llvm::AllocaInst *valPtr = _builder.CreateAlloca(
			_host.ObjectType(), 0, valName );
	string endName = "assert_end_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *end = CreateBasicBlock( endName );

	string successName = "assert_true_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *successBranch = CreateBasicBlock( successName );
	string failName = "assert_false_" + numtostr_dec( _blockNum );
	llvm::BasicBlock *failBranch = CreateBasicBlock( failName );
	llvm::Value *booleanToBoolFunc = _module->getOrInsertFunction(
			kIntrinsicBooleanToBoolFunctionName,
			_host.BoolType(),	// return type
			_host.ZoneType(),   // alloc zone parameter
			_host.ObjectType(), // input parameter
			NULL );
	args.clear();
	args.push_back( GetImplicitZone() );
	args.push_back( condition );
	llvm::Value *cond_bool = _builder.CreateCall( booleanToBoolFunc, args );
	_builder.CreateCondBr( cond_bool, successBranch, failBranch );

	// If success, we'll return the condition, which is "true".
	_builder.SetInsertPoint( successBranch );
	_builder.CreateStore( condition, valPtr );
	_builder.CreateBr( end );

	// If failure, we'll raise the message as an exception.
	_builder.SetInsertPoint( failBranch );
	args.clear();
	llvm::Value *throwFunc = _module->getOrInsertFunction(
			kIntrinsicThrowFunctionName,
			_host.ObjectType(),	// return type
			_host.ZoneType(), // where to allocate objects
			_host.ObjectType(), // input parameter
			NULL );
	args.clear();
	args.push_back( GetImplicitZone() );
	args.push_back( message );
	llvm::Value *exception = _builder.CreateCall( throwFunc, args );
	_builder.CreateStore( exception, valPtr );
	_builder.CreateBr( end );

	// Either way, the value which ended up in our var is the result.
	_builder.SetInsertPoint( end );
	return _builder.CreateLoad( valPtr, valName );
}

llvm::Value *Atom::ProcessChain( const Op& it )
{
	assert( it.Code() == Op::Code::Chain );
	vector<llvm::Value *> args;
	// The left operand is the existing assert chain value. The right operand
	// is a value we would like to append on to the assert chain. If the left
	// value is an exception, we will return it; otherwise, we will return the
	// right value. This way later exceptions don't cover the messages from
	// earlier exceptions.

	llvm::Value *head = Load( it.Left() );
	llvm::Value *tail = Load( it.Right() );
	llvm::Value *exTestFunc = _module->getOrInsertFunction(
			kIntrinsicExceptionCheckFunctionName,
			_host.BoolType(),		// return type
			_host.ObjectType(),			// input parameter
			NULL );
	args.clear();
	args.push_back( head );
	llvm::Value *isException = _builder.CreateCall( exTestFunc, args );
	return _builder.CreateSelect( isException, head, tail );
}

llvm::Value *Atom::ProcessCall( const Op& it )
{
	assert( it.Code() == Op::Code::Call );
	llvm::Value *target = Load( it.Target() );
	auto gep = _builder.CreateStructGEP( target, 0 );
	llvm::Value *function = _builder.CreateLoad( gep );
	vector<llvm::Value *> args;
	args.push_back( GetImplicitZone() );
	args.push_back( target );
	args.push_back( _builder.getInt32( it.Args().size() ) );

	for (auto arg: it.Args()) {
		args.push_back( Load( arg ) );
	}
	
	return _builder.CreateCall( function, args );
}

llvm::Value *Atom::ProcessCapture( const Op& it )
{
	assert( it.Target().Type() == Addr::Type::Link );
	llvm::Type *funcType = _host.FunctionType()->getPointerTo();
	llvm::Value *allocObject = _module->getOrInsertFunction(
			"alloc_object", 
			_host.ObjectType(), 
			_host.ZoneType(),
			funcType, 
			_builder.getInt32Ty(),
			NULL);
	llvm::Value *zone = GetImplicitZone();
	llvm::Value *func = _module->getFunction( it.Target().Link() );
	llvm::Value *argCount = 
			llvm::ConstantInt::get( _builder.getInt32Ty(), it.Args().size() );
	
	// Invoke alloc_object to create our new object. Note that we need to cast
	// the function to match the type of object_alloc exactly.
	func = _builder.CreateBitCast( func, funcType  );
	llvm::Value *result = 
			_builder.CreateCall3( allocObject, zone, func, argCount );

	// See Op::Code::Slot for an explaination of this GEP magic.
	llvm::Value *slots = _builder.CreateStructGEP( result, 1 );	
	for(unsigned i = 0; i < it.Args().size(); i++) {
		llvm::Value *slot = _builder.CreateConstInBoundsGEP2_32( slots, 0, i );
		llvm::Value *value = Load( it.Args()[i] );
		_builder.CreateStore( value, slot );
	}

    return result;
}


void Atom::Process( const Op& it )
{
	assert( it.Dest().Type() == Addr::Type::Register );
	
	// This isn't required, but putting each op in its own basic block makes
	// the resulting assembly so much easier to read.
	string blockName = "op" + numtostr_dec( _blockNum++ ) + "_" + it.Name();
	llvm::BasicBlock *block = CreateBasicBlock( blockName );
	_builder.CreateBr( block );
	_builder.SetInsertPoint( block );

    llvm::Value *val = NULL;
	switch (it.Code()) {
		case Op::Code::Self: val = ProcessSelf( it ); break;
		case Op::Code::NumberLiteral: val = ProcessNumberLiteral( it ); break;
		case Op::Code::FloatLiteral: val = ProcessFloatLiteral( it ); break;
		case Op::Code::StringLiteral: val = ProcessStringLiteral( it ); break;
		case Op::Code::SymbolLiteral: val = ProcessSymbolLiteral( it ); break;
		case Op::Code::Parameter: val = ProcessParameter( it ); break;
		case Op::Code::Slot: val = ProcessSlot( it ); break;
		case Op::Code::Import: val = ProcessImport( it ); break;
		case Op::Code::Repeat: val = ProcessRepeat( it ); break;
		case Op::Code::LoopWhile: val = ProcessLoopWhile( it ); break;
		case Op::Code::Assert: val = ProcessAssert( it ); break;
		case Op::Code::Chain: val = ProcessChain( it ); break;
		case Op::Code::Call: val = ProcessCall( it ); break;
		case Op::Code::Capture: val = ProcessCapture( it ); break;
		default: assert(0);
	}
    Store( val, it.Dest(), VarName( it ) );
}

std::string Atom::VarName( const LIC::Op &it )
{
    // Make up a better name for this var than the default.
    // Return empty string to use the default.
    switch (it.Code()) {
        case Op::Code::Self: return "self";
        case Op::Code::Parameter:
                return "arg" + numtostr_dec(it.Value().Index() );
        case Op::Code::Slot:
                return "slot" + numtostr_dec(it.Value().Index() );
        default: return "";
    }
}

llvm::Value * Atom::GetParameter( int index )
{
	// The arg iterator isn't a random access iterator, so we'll have to do
	// this the fun way. (At least, I think we need to do it like this).
	llvm::Function::arg_iterator iter = _function->arg_begin();
	for (int i = 0; i < index; ++i) { 
		++iter; 
		assert(iter != _function->arg_end()); 
	}
	return iter;
}

llvm::Value * Atom::GetImplicitSelf()
{
	return GetParameter( kImplicitSelfParameterIndex );
}

llvm::Value * Atom::GetImplicitZone()
{
	return GetParameter( kImplicitZoneParameterIndex );
}

llvm::Value * Atom::GetImplicitArgc()
{
	return GetParameter( kImplicitArgcParameterIndex );
}
