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

#ifndef ast_flowfuncs_h
#define ast_flowfuncs_h

#include "flowgraph/node.h"
#include "utility/numtostr.h"

namespace Flowgraph {

class Self : public Node
{
	friend class Pool;
	public:
		bool IsSelf() const { return true; }
	protected:
		Self() : Node() {}
		void FormatString( NodeFormatter *formatter ) const;
};

class Function : public Node
{
	friend class Pool;
	public:
		bool IsAFunction() const { return true; }
		Function *AsFunction() { return this; }
		virtual bool IsContextIndependent() const { return true; }
		Node *Exp() const { return _exp; }
		unsigned int Arity() const { return _arity; }
		std::string Name() const { return _name; }
		std::string ToString() const;
	protected:
		Function( Node *exp, unsigned int arity );
		Function( Node *exp, unsigned int arity, std::string name );
		std::string DefaultName() const;
		void FormatString( NodeFormatter *formatter ) const;
	private:
		Node *_exp;
		unsigned int _arity;
		std::string _name;
};

class Parameter : public Node
{
	friend class Pool;
	public:
		bool IsAParameter() const { return true; }
		Parameter *AsParameter() { return this; }
		unsigned int Index() const { return _index; }
		unsigned int MinParameterCount() const { return _index + 1; }
	protected:
		Parameter( unsigned int index ) : Node(), _index(index) {}
		void FormatString( NodeFormatter *formatter ) const;
	private:
		unsigned int _index;
};

class Slot : public Node
{
	friend class Pool;
	public:
		bool IsASlot() const { return true; }
		Slot *AsSlot() { return this; }
		unsigned int Index() { return _index; }
	protected:
		Slot( unsigned int index ) : Node(), _index(index) {}
		void FormatString( NodeFormatter *formatter ) const;
	private:
		unsigned int _index;
};

} // namespace Flowgraph

#endif // flowfuncs_h
