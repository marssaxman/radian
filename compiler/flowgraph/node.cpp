// Copyright 2016 Mars Saxman.
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

#include "flowgraph/node.h"
#include "flowgraph/pool.h"

using namespace Flowgraph;

std::string Node::ToString() const
{
	NodeFormatter format;
	FormatString( &format );
	return format.Result();
}

Import::Import( Node *fileName, Node *sourceDir ) :
	Node(),
	_fileName(fileName),
	_sourceDir(sourceDir)
{
	assert( fileName );
	assert( sourceDir );
}

void Import::FormatString( NodeFormatter *formatter ) const
{
	formatter->Begin( "import" );
	_fileName->FormatString( formatter );
	_sourceDir->FormatString( formatter );
	formatter->End();
}

Inductor::Inductor( Node *exp ) :
	Node(),
	_exp(exp)
{
	assert( exp && exp->IsAnOperation() );
}

void Inductor::FormatString( NodeFormatter *formatter ) const
{
	_exp->FormatString( formatter );
}

void Placeholder::FormatString( NodeFormatter *formatter ) const
{
	formatter->Element( "placeholder_" + numtostr_dec( _index ) );
}

NodeFormatter::NodeFormatter() :
	_indentLevel(0)
{
}

void NodeFormatter::Begin( std::string id )
{
	// Enter a new group. If we already have a group open, we'll turn it into
	// a multiline group, dumping each preceding element out on its own line.
	// The new group will take over the item queue. Only the innermost groups
	// will fit on a single line.
	if (!_items.empty()) {
		bool hasIndented = false;
		while (!_items.empty()) {
			_result += Tabs() + _items.front() + "\n";
			_items.pop();
			if (!hasIndented) {
				_indentLevel++;
				hasIndented = true;
			}
		}
	}
	_items.push( "(" + id );
}

void NodeFormatter::End()
{
	// If the queue is empty, that means this block has gone into multiline
	// mode, and all preceding items have been written out individually. If
	// there are items on the queue, that means no other blocks have begun
	// inside this one, so all the elements are still on a single line.
	if (_items.empty()) {
		assert( _indentLevel > 0 );
		_indentLevel--;
	}
	_result += Tabs();
	std::string local;
	while (!_items.empty()) {
		if (!local.empty()) local += " ";
		local += _items.front();
		_items.pop();
	}
	_result += local + ")\n";
}

void NodeFormatter::Element( std::string id )
{
	// If we have a queue going, add this item to it. Otherwise, write the
	// item out on its own line, as all previous elements must have done.
	if (!_items.empty()) {
		_items.push( id );
	} else {
		_result += Tabs() + id + "\n";
	}
}

std::string NodeFormatter::Tabs() const
{
	std::string out;
	for (unsigned int i = 0; i < _indentLevel; i++) {
		out += "\t";
	}
	return out;
}

// Flowgraph::Rewrite
//
// The sources of the values in this expression (may have) changed. Traverse
// the graph and rewrite any nodes that depend on changed source values. Idea
// for future improvement: define some placeholder node whose meaning can be
// defined upstream by some kind of definition node, then have the PostOrderDFS
// perform the substitution during traversal. This would free us from the need
// to traverse the graph twice and to reallocate the intermediary nodes - this
// could be a big memory win given that we will likely need to recreate the
// majority of the loop body. Note that we fill out the map as we go, so we
// don't re-traverse any shared subexpressions.
//
Node *Flowgraph::Rewrite( Node *exp, Pool &pool, NodeMap &reMap )
{
	NodeMap::const_iterator iter = reMap.find(exp);
	if (iter != reMap.end()) {
		exp = iter->second;
	}
	else if (exp->IsAnOperation()) {
		Operation *op = exp->AsOperation();
		Node *left = Rewrite( op->Left(), pool, reMap );
		Node *right = Rewrite( op->Right(), pool, reMap );
		Node *newExp = pool.Operation( op->Type(), left, right );
		if (exp->IsInductionVar() && !op->IsInductionVar()) {
			newExp = pool.Inductor( newExp );
		}
		reMap[exp] = newExp;
		exp = newExp;
	}
	return exp;
}

