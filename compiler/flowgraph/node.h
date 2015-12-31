// Copyright 2009-2013 Mars Saxman.
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


#ifndef node_h
#define node_h

#include <string>
#include <stdint.h>
#include <set>
#include <map>
#include <queue>
#include <assert.h>
#include "error.h"
#include "numtostr.h"

namespace Flowgraph {

class Node
{
	friend class Pool;
	friend class Cache;
	public:
		std::string UniqueID() const { return numtostr_hex((uintptr_t)this); }

		virtual bool IsVoid() const { return false; }
		virtual bool IsSelf() const { return false; }
		virtual bool IsAValue() const { return false; }
		virtual bool IsASymbol() const { return false; }
		virtual class Value *AsValue() { assert(false); return NULL; }
		virtual bool IsAFunction() const { return false; }
		virtual class Function *AsFunction() { assert(false); return NULL; }
		virtual bool IsAnOperation() const { return false; }
		virtual class Operation *AsOperation() { assert(false); return NULL; }
		virtual bool IsAnImport() const { return false; }
		virtual class Import *AsImport() { assert(false); return NULL; }
		virtual bool IsAParameter() const { return false; }
		virtual class Parameter *AsParameter() { assert(false); return NULL; }
		virtual bool IsASlot() const { return false; }
		virtual class Slot *AsSlot() { assert(false); return NULL; }
		virtual bool IsAnArg() const { return false; }
		virtual bool IsIntrinsic() const { return false; }
		virtual class Intrinsic *AsIntrinsic() { assert(false); return NULL; }
		virtual bool IsAPlaceholder() const { return false; }
		virtual bool IsACapture() const { return false; }
		virtual std::string ToString() const;
		virtual void FormatString( class NodeFormatter *formatter ) const = 0;

		// is this node an induction variable? that is, does it depend only on
		// loop invariants and on other induction variables?
		virtual bool IsInductionVar() const { return false; }
		virtual bool IsPrimeInductor() const { return false; }
		// is this node trivially hoistable? that is, can it be evaluated
		// independently of any particular execution context?
		virtual bool IsContextIndependent() const { return false; }
		// what is the minimum number of parameters this expression depends on?
		// that is, highest index plus one
		virtual unsigned int MinParameterCount() const { return 0; }

	protected:
		Node() {}
		virtual ~Node() {}
};

typedef std::set<Flowgraph::Node*> NodeSet;
typedef std::map<Flowgraph::Node*,Flowgraph::Node*> NodeMap;

Node *Rewrite( Node *exp, class Pool &pool, NodeMap &reMap );

class Import : public Node
{
	friend class Pool;
	public:
		bool IsAnImport() const { return true; }
		Import *AsImport() { return this; }
		Node *FileName() const { return _fileName; }
		Node *SourceDirectory() const { return _sourceDir; }
		bool IsContextIndependent() const { return true; }

	protected:
		Import( Node *fileName, Node *sourceDir );
		void FormatString( class NodeFormatter *formatter ) const;
		Node *_fileName;
		Node *_sourceDir;
};

class Inductor : public Node
{
	friend class Pool;
	public:
		bool IsInductionVar() const { return true; }
		bool IsPrimeInductor() const { return true; }
		virtual bool IsAnOperation() const { return true; }
		virtual class Operation *AsOperation() { return _exp->AsOperation(); }
		virtual bool IsAnArg() const { return _exp->IsAnArg(); }

	protected:
		Inductor( Node *exp );
		void FormatString( class NodeFormatter *formatter ) const;
		Node *_exp;
};

class Placeholder : public Node
{
	friend class Pool;
	public:
		virtual bool IsAPlaceholder() const { return true; }

	protected:
		Placeholder( unsigned int index ) : _index(index) {}
		void FormatString( class NodeFormatter *formatter ) const;
		unsigned _index;

};

class NodeFormatter
{
	public:
		NodeFormatter();
		void Begin( std::string id );
		void Element( std::string val );
		void End();
		std::string Result() const { return _result; }
	private:
		std::string Tabs() const;
		std::queue<std::string> _items;
		std::string _result;
		unsigned int _indentLevel;
};

} // namespace Flowgraph

#endif //node_h
