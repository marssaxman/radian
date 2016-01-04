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

#ifndef flowgraph_postorderdfs_h
#define flowgraph_postorderdfs_h

#include <set>
#include <stack>
#include "utility/sequence.h"
#include "flowgraph/flowgraph.h"

// Most primitive component in the graphworks system, this implements - as its
// name implies - a post-order, depth-first traversal of a node graph. Since it
// uses a visited list, the graph need not be acyclic. 
class PostOrderDFS : public Iterator<Flowgraph::Node*>
{
	public:
		PostOrderDFS( Flowgraph::Node *root );
		Flowgraph::Node *Current() const;
		bool Next();
	protected:
		void Push( Flowgraph::Node *item );
		bool Visited( Flowgraph::Node *item ) const;
		virtual bool Viewable( Flowgraph::Node *item ) const { return true; }
	private:
		Flowgraph::Node *_root;
		std::set<Flowgraph::Node*> _visited;
		std::stack<Flowgraph::Node*> _worklist;
};

#endif //postorderdfs_h
