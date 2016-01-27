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

#ifndef linearcode_linearcode_h
#define linearcode_linearcode_h

#include <string>
#include <deque>
#include <assert.h>
#include "linearcode/licaddr.h"

namespace LIC {

class Op
{
	public:
		struct Code { enum Enum {
			// No operands
			Self,
		
			// One data operand - MonOp
			NumberLiteral,
			FloatLiteral,
			StringLiteral,
			SymbolLiteral,
			Parameter,
			Slot,
			Import,
			Repeat,				// end of loop block

			// Two register operand - BinOp
			LoopWhile,			// beginning of loop block
			Assert,				// return true or throw exception
			Chain,				// return true or one of two exceptions

			// Target plus args - TargetOp
			Call,
			Capture,

			COUNT
		}; };

		virtual ~Op() {}
		std::string Name() const;
		virtual std::string ToString() const = 0;
		virtual Addr Value() const
				{ assert( false ); return Addr::NilInstance(); }
		virtual Addr Left() const
				{ assert( false ); return Addr::NilInstance(); }
		virtual Addr Right() const
				{ assert( false ); return Addr::NilInstance(); }
		virtual Addr Target() const
				{ assert( false ); return Addr::NilInstance(); }
		virtual const std::deque<Addr> &Args() const;
		virtual Op * Clone() const { assert(false); return NULL; }
		
		Code::Enum Code() const { return _op; }
		Addr Dest() const { return _dest; }

	protected:
		Op( Addr dest, Code::Enum op ) :
			_dest(dest),
			_op(op) {}

	private:
		Addr _dest;
		Code::Enum _op;
};

class SelfOp : public Op
{
	public:
		SelfOp( Addr dest ) :
			Op( dest, Code::Self ) {}
		std::string ToString() const;
		Op * Clone() const;
};

class MonOp : public Op
{
	public:
		~MonOp() {}
		MonOp( Addr dest, Code::Enum op, Addr value );
		Addr Value() const { return _value; }
		std::string ToString() const;
		Op * Clone() const;

	private:
		Addr _value;
};

class BinOp : public Op
{
	public:
		~BinOp() {}
		BinOp( Addr dest, Code::Enum op, Addr left, Addr right);
		Addr Left() const { return _left; }
		Addr Right() const { return _right; }
		std::string ToString() const;
		Op * Clone() const;

	private:
		Addr _left;
		Addr _right;

};

class TargetOp : public Op
{
	public:
		~TargetOp() {}
		TargetOp( Addr dest, Code::Enum op, Addr target, const std::deque<Addr> &args );
		Addr Target() const { return _target; }
		const std::deque<Addr> &Args() const { return _args; }
		std::string ToString() const;	
		Op * Clone() const;

	private:
		std::deque<Addr> _args;
		Addr _target;
};

} // namespace LIC

#endif //linearcode_h
