// Copyright 2009-2014 Mars Saxman.
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


#include "linearcode.h"
#include "numtostr.h"
#include <assert.h>

using namespace std;
using namespace LIC;

string Op::Name() const
{
	static string opcodes[Op::Code::COUNT] = {
		"Self",
		"NumberLiteral",
        "NumberFromDouble",
		"StringLiteral",
		"SymbolLiteral",
		"Parameter",
		"Slot",
		"Import",
		"Repeat",
		"LoopWhile",
		"Assert",
		"Chain",
		"Call",
		"Capture"
	};
	assert( _op >= 0 && _op < Code::COUNT );
	return opcodes[_op];	
}

const std::deque<Addr> &Op::Args() const
{
	assert( false );
	static const std::deque<Addr> ret;
	return ret;
}


string SelfOp::ToString() const
{
	return Dest().ToString() + " = self";
}

Op * SelfOp::Clone() const 
{ 
	return new SelfOp( Dest() ); 
}

MonOp::MonOp( Addr dest, Code::Enum op, Addr value ) : 
	Op(dest, op),
	_value(value) 
{
	assert( 
		op == Code::NumberLiteral ||
		op == Code::FloatLiteral ||
		op == Code::StringLiteral ||
		op == Code::SymbolLiteral ||
		op == Code::Parameter ||
		op == Code::Slot ||
		op == Code::Import ||
		op == Code::Repeat );
}

string MonOp::ToString() const
{
	string out = Dest().ToString() + " = " + Name();
	out += " " + _value.ToString();
	return out;
}

Op * MonOp::Clone() const 
{ 
	return new MonOp( Dest(), Code(), Value() ); 
}

BinOp::BinOp( Addr dest, Code::Enum op, Addr left, Addr right) :
	Op(dest, op),
	_left(left),
	_right(right) 
{
	assert(
		op == Code::LoopWhile ||
		op == Code::Assert ||
		op == Code::Chain );
}

string BinOp::ToString() const
{
	string out = Dest().ToString() + " = " + Name();
	out += "(" + _left.ToString() + ", " + _right.ToString() + ")";
	return out;
}

Op * BinOp::Clone() const 
{ 
	return new BinOp( Dest(), Code(), Left(), Right() ); 
}

TargetOp::TargetOp(
		Addr dest, Code::Enum op, Addr target, const std::deque<Addr> &args ) :
	Op(dest, op),
	_args(args),
	_target(target)
{
	assert(
		op == Code::Call ||
		op == Code::Capture );
}

string TargetOp::ToString() const
{
	string out = Dest().ToString() + " = " + Name() + " ";
	out += _target.ToString() + "(";
	for (std::deque<Addr>::const_iterator iter = Args().begin();
			iter < Args().end();
			iter++) {
		if (iter != Args().begin()) out += ", ";
		out += (*iter).ToString();
	}
	out += ")";
	return out;
}

Op * TargetOp::Clone() const
{ 
	return new TargetOp( Dest(), Code(), Target(), Args() ); 
}
