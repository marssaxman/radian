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

#ifndef linearcode_licaddr_h
#define linearcode_licaddr_h

#include <string>
#include <assert.h>

namespace LIC {

class Addr
{
	public:	
		// For use with the nil object pattern, but mostly to get the compiler
		// to stop barking about not having valid return values.
		static const Addr NilInstance() { static Addr addr; return addr; }

		struct Type { enum Enum {
			Void,
			Data,
			Register,
			Link,
			Index,
			Intrinsic
		}; };
		Addr();
		Addr( unsigned int register );
		Addr( std::string data );
		Addr( const Addr& other );
		static Addr Link( std::string id ) 
			{ Addr out( id ); out._type = Type::Link; return out; }
		static Addr Intrinsic( std::string id ) 
			{ Addr out( id ); out._type = Type::Intrinsic; return out; }
		static Addr Index( unsigned int index ) 
			{ Addr out( index ); out._type = Type::Index; return out; }
		std::string ToString() const;

		Type::Enum Type() const { return _type; }
		unsigned int Register() const 
			{ assert( _type == Type::Register ); return _register; }
		std::string Data() const 
			{ assert( _type == Type::Data ); return _data; }
		std::string Link() const 
			{ assert( _type == Type::Link ); return _data; }
		std::string Intrinsic() const 
			{ assert( _type == Type::Intrinsic ); return _data; }
		unsigned int Index() const
			{ assert( _type == Type::Index ); return _register; }
		
		void Dump() const;
	private:
		Type::Enum _type;
		std::string _data;
		unsigned int _register;
};

} // namespace LIC

#endif //licaddr_h
