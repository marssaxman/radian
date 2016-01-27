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

#ifndef flowgraph_value_h
#define flowgraph_value_h

#include <string>
#include "flowgraph/node.h"

namespace Flowgraph {

class Value : public Node
{
    friend class Pool;
    public:
        struct Type { enum Enum {
            Void,
            Number,	// exact number
			Float,	// approximate number
            String,
            Symbol
        }; };
        Type::Enum Type() const { return _type; }
        std::string Contents() const { return _value; }
        bool HasError() const { return false; }
        virtual bool IsAValue() const { return true; }
		virtual bool IsASymbol() const { return _type == Type::Symbol; }
		virtual bool IsVoid() const { return _type == Type::Void; }
		virtual bool IsContextIndependent() const { return true; }
		virtual Value *AsValue() { return this; }
    protected:
        Value( Type::Enum code, std::string value );
		void FormatString( NodeFormatter *formatter ) const;    
        
        std::string _value;
        Type::Enum _type;
};

} // namespace Flowgraph

#endif // value_h
