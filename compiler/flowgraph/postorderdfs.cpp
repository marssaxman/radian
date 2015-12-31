// Copyright 2009-2012 Mars Saxman.
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


#include "postorderdfs.h"
#include "operation.h"

using namespace Flowgraph;

PostOrderDFS::PostOrderDFS( Node *root ):
	_root(root)
{
	assert( root );
}

bool PostOrderDFS::Next()
{
	if (_root) {
		Push( _root );
		_root = NULL;
	} else if (!_worklist.empty()) {
		// pop the current item from the work list, since the caller has
		// already looked at it. If that was the last item we found, then we
		// are finished.
		assert( Visited( _worklist.top() ) );
		_worklist.pop();
	}
	if (_worklist.empty()) return false;
	// look at the new top of the work list. if it is an operation, we expect
	// that we have already pushed its left node, since that's how the push
	// works. the question, then, is whether its right node has also been
	// visited; if so, we will leave the list top where it is, and it will be
	// our new current value. If its right node has not been visited, we'll dig
	// into that.
	Node *item = _worklist.top();
	assert( item && !Visited( item ) && Viewable( item ) );
	if (item->IsAnOperation()) {
		Operation *op = item->AsOperation();
		assert( Visited( op->Left() ) || !Viewable( op->Left() ) );
		Push( op->Right() );
	}
	// Mark whichever node ended up at the top of our worklist as visited,
	// since it is now our current node.
	assert( !_worklist.empty() && !Visited( _worklist.top() ) );
	_visited.insert( Current() );
	return true;
}

Node *PostOrderDFS::Current() const 
{
	Node *out = _root;
	if (!_worklist.empty()) {
		out = _worklist.top();
		assert( out );
		assert( Viewable( out ) );
		if (out->IsAnOperation()) {
			Operation *op = out->AsOperation();
			assert( Visited( op->Left() ) || !Viewable( op->Left() ) );
			assert( Visited( op->Right() ) || !Viewable( op->Right() ) );
		}
	}
	return out;
}

void PostOrderDFS::Push( Node *item )
{
	while (!Visited( item ) && Viewable( item )) {
		_worklist.push( item );
		if (!item->IsAnOperation()) break;
		Operation *op = item->AsOperation();
		if (!Visited( op->Left() ) && Viewable( op->Left() )) {
			item = op->Left();
		} else {
			item = op->Right();
		}
	}
}

bool PostOrderDFS::Visited( Node *item ) const
{
	assert( item );
	return _visited.find( item ) != _visited.end();
}
