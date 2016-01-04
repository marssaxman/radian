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

#include <stdio.h>
#include "linearcode/linearizer.h"
#include "flowgraph/flowgraph.h"
#include "utility/numtostr.h"
#include "flowgraph/postorderdfs.h"

using namespace Flowgraph;
using namespace LIC;

Linearizer::Linearizer( Iterator<Node*> &source ):
	_source(source),
	_regCount(0)
{
}

Linearizer::~Linearizer()
{
	while (!_current.empty()) {
		delete _current.front();
		_current.pop();
	}
}

LIC::Addr Linearizer::Result() const
{ 
	// This should never be void in valid code, but if we are processing 
	// invalid code, we may end up passing through here after reporting some
	// errors. Therefore we cannot assert that the result is non-void.
	return _result; 
}

LIC::Op *Linearizer::Current() const
{
	if (_current.empty()) return NULL;
	return _current.front();
}


bool Linearizer::Next()
{
	if (!_current.empty()) {
		delete _current.front();
		_current.pop();
	}
	while (_current.empty() && _source.Next()) {
		Node *it = _source.Current();
		// We must save off the result first, because we may not have any
		// LIC::Op to emit for this node.
		_result = FindNode( it );
		Process( it );
	}
	return !_current.empty();
}

// Linearizer::Process
//
// Attempt to turn this node into a LIC::Op. If we succeed, write the ops out
// to the _current queue.
//
void Linearizer::Process( Node *it )
{
	if (it->IsVoid()) return;
	if (it->IsIntrinsic()) return;
	if (it->IsAnArg()) return;		
	if (it->IsAFunction()) return;
	if (it->IsAnOperation()) {
		ProcessOperation(it->AsOperation());
	}
	else if (it->IsAValue()) {
		Value *val = it->AsValue();
		Op::Code::Enum opname;
		std::string opvalue;
		switch (val->Type()) {
			case Value::Type::Number: opname = Op::Code::NumberLiteral; break;
			case Value::Type::Float: opname = Op::Code::FloatLiteral; break;
			case Value::Type::String: opname = Op::Code::StringLiteral; break;
			case Value::Type::Symbol: opname = Op::Code::SymbolLiteral; break;
			default: assert( false );
		}
		_current.push( new MonOp( _result, opname, val->Contents() ) );
	} 
	else if (it->IsAParameter()) {
		Addr index = Addr::Index( it->AsParameter()->Index() );
		_current.push( new MonOp( _result, Op::Code::Parameter, index ) );
	} 
	else if (it->IsASlot()) {
		Addr index = Addr::Index( it->AsSlot()->Index() );
		_current.push( new MonOp( _result, Op::Code::Slot, index ) );
	} 
	else if (it->IsAnImport()) {
		std::string ident = it->AsImport()->FileName()->AsValue()->Contents();
		_current.push( new MonOp( _result, Op::Code::Import, ident ) );
	} 
	else if (it->IsSelf()) {
		_current.push( new SelfOp( _result ) );
	} 
	else {
		fprintf(
				stderr, 
				"linearizing unknown node type: gonna die now. \n\t%s\n", 
				it->ToString().c_str() );
		assert( false );
	}
}

void Linearizer::ProcessOperation( Operation *op )
{
	switch (op->Type()) {
		case Operation::Type::Call: {
			ProcessCall( op );
		} break;
		case Operation::Type::Capture: {
			// Capture a function, yielding an invokable
			std::deque<Addr> args;
			Node *arg = op->Right();
			while (arg->IsAnArg()) {
				args.push_front( FindNode( arg->AsOperation()->Right() ) );
				arg = arg->AsOperation()->Left();
			}
			_current.push( new TargetOp(
					_result,
					Op::Code::Capture,
					FindNode( op->Left() ),
					args ));
		} break;
		case Operation::Type::Loop: {
			// do nothing with this, since we're going to handle it when we get
			// to the part where we call it.
		} break;
		case Operation::Type::Arg: {
			// this should never happen, because we should only handle args
			// under operations which define some meaning for them; they should
			// never exist on their own.
			assert(false);
		} break;
		case Operation::Type::Assert: {
			// make sure some condition is true, or raise an exception
			_current.push( new BinOp(
					_result,
					Op::Code::Assert,
					FindNode( op->Left() ),
					FindNode( op->Right() )));
		} break;
		case Operation::Type::Chain: {
			// append a value onto an assert chain
			_current.push( new BinOp(
					_result,
					Op::Code::Chain,
					FindNode( op->Left() ),
					FindNode( op->Right() )));
		} break;
		default: assert(false);
	}
}

void Linearizer::ProcessCall( Operation *op )
{
	assert( Operation::Type::Call == op->Type() );
	if (op->Left()->IsAnOperation() &&
			op->Left()->AsOperation()->Type() == Operation::Type::Loop) {
		// Call a loop: let's inline the loop
		// The loop operation creates a loop function, which we then call with
		// some initial value. We could render that directly, but it's better
		// to use the LoopWhile / Repeat instructions to run the loop inline.
		Operation *loop = op->Left()->AsOperation();
		Addr condition = FindNode( loop->Left() );
		Addr operation = FindNode( loop->Right() );
		Operation *callarg = op->Right()->AsOperation();
		assert( callarg->Type() == Operation::Type::Arg );
		assert( callarg->Left()->IsVoid() );
		Addr startval = FindNode( callarg->Right() );
		Addr tempA = AllocRegister();
		Addr tempB = AllocRegister();
		_current.push(
				new BinOp( tempA, Op::Code::LoopWhile, startval, condition ) );
		std::deque<Addr> args;
		args.push_front( tempA );
		_current.push(
				new TargetOp( tempB, Op::Code::Call, operation, args ) );
		_current.push(
				new MonOp( _result, Op::Code::Repeat, tempB ) );
	} else {
	// Call anything else: generate a normal call
		std::deque<Addr> args;
		Node *arg = op->Right();
		while (arg->IsAnArg()) {
			args.push_front( FindNode( arg->AsOperation()->Right() ) );
			arg = arg->AsOperation()->Left();
		}
		_current.push( new TargetOp(
				_result,
				Op::Code::Call,
				FindNode( op->Left() ),
				args ));
	}
}

Addr Linearizer::FindNode( Node *which )
{
	// Have we already encountered this node? If so, we will have already saved
	// a register instance for it, so we can reuse that. Otherwise, we'll have
	// to make up an appropriate register for this node and add it to our map.
	std::map<Flowgraph::Node *, Addr>::iterator found = _addrMap.find( which );
	if (found != _addrMap.end()) {
		return found->second;
	} else {
		Addr out;
		if (which->IsAFunction()) {
			out = Addr::Link( which->AsFunction()->Name() );
		} else if (which->IsIntrinsic()) {
			out = Addr::Intrinsic( which->AsIntrinsic()->Link() );
		} else if (!which->IsVoid()) {
			out = AllocRegister();
		}
		_addrMap[which] = out;
		return out;
	}
}

Addr Linearizer::AllocRegister()
{
	return Addr( _regCount++ );
}


// Linearizer::ToString
//
// Debug output formatter: creates a big string with the contents of this
// function, using some minimal formatting. This is a static method on the
// Linearizer because there is no object which represents a LIC function. 
//
std::string Linearizer::ToString( Function *func )
{
	std::string out;
	PostOrderDFS podfs(func->Exp());
	Linearizer l(podfs);
	out += func->Name() + ":\n";
	while (l.Next()) {
		out += "    " + l.Current()->ToString() + "\n";
	}
	out += "    return " + l.Result().ToString() + "\n";
	return out;
}
