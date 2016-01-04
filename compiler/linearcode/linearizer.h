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

#ifndef linearcode_linearizer_h
#define linearcode_linearizer_h

#include <map>
#include <string>
#include <vector>
#include <queue>
#include "utility/sequence.h"
#include "linearcode/linearcode.h"
#include "flowgraph/flowgraph.h"

// Emit a series of LIC operations performing the computation described in the
// input, which should be a stream of nodes representing one function in
// depth-first order.
class Linearizer : public Iterator<LIC::Op*>
{
	public:
		Linearizer( Iterator<Flowgraph::Node*> &source );
		~Linearizer();
		bool Next();
		LIC::Op *Current() const;
		LIC::Addr Result() const;
		static std::string ToString( Flowgraph::Function *func ); 
		
	protected:
		LIC::Addr FindNode( Flowgraph::Node *it );
		void Process( Flowgraph::Node *it );
		void ProcessOperation( Flowgraph::Operation *it );
        void ProcessCall( Flowgraph::Operation *it );
        LIC::Addr AllocRegister();
		
	private:
        std::queue<LIC::Op*> _current;
		LIC::Addr _result;
		Iterator<Flowgraph::Node*> &_source;
		unsigned int _regCount;
		std::map<Flowgraph::Node*, LIC::Addr> _addrMap;
};

#endif //linearizer_h
