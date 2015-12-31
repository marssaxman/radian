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


#ifndef operation_h
#define operation_h

#include "node.h"

namespace Flowgraph {

class Operation : public Node
{
	friend class Pool;
	public:
		struct Type { enum Enum {
			Call,				// call object, argument
			Capture,			// capture context, function
			Arg,				// arg-list head, tail
			Loop,				// loop condition, operation
			Assert,				// assert condition, message
			Chain,				// chain head, tail
			COUNT
		}; };
	
		Type::Enum Type() const { return _type; }
		Node *Left() const { return _left; }
		Node *Right() const { return _right; }
		virtual bool IsAnOperation() const { return true; }
		virtual Operation *AsOperation() { return this; }
		virtual bool IsAnArg() const { return _type == Type::Arg; }
		virtual bool IsACapture() const { return _type == Type::Capture; }
		virtual bool IsInductionVar() const { return _isInductionVar; }
		virtual unsigned int MinParameterCount() const
				{ return _minParameterCount; }

	protected:
		Operation( Type::Enum which, Node *left, Node *right );
		virtual void FormatString( NodeFormatter *formatter ) const;
		bool FormatMethodCallPattern( NodeFormatter *formatter ) const;
	
	private:
		Type::Enum _type;
		Node *_left;
		Node *_right;
		bool _isInductionVar;
		unsigned int _minParameterCount;
};

} // namespace Flowgraph

#endif // operation_h
