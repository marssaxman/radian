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


#ifndef unicode_h
#define unicode_h

#if WIN32
	typedef unsigned int uchar_t;
#else
	#include <stdint.h>
	typedef uint32_t uchar_t;
#endif
#include <string>

namespace Unicode {

class Iterator
{
	public:
		Iterator( std::string src );
		virtual ~Iterator() {}
        bool Done() { return _offsetBytes >= _source.size(); }
		virtual uchar_t Next() = 0;
        unsigned int Position() { return _offsetBytes; }
        void MoveTo( unsigned int pos );

        static Iterator *GuessEncoding( std::string src );
    protected:
        std::string _source;
        unsigned int _offsetBytes;
};

class Writer
{
	public:
		Writer() {}
		virtual ~Writer() {}
		void Reset() { _value.clear(); }
		virtual void Append( uchar_t ch ) = 0;
		std::string Value() const { return _value; }
	protected:
		std::string _value;
};

} // namespace Unicode


namespace UTF8 {

class Iterator : public Unicode::Iterator
{
    public:
        Iterator( std::string src ) : Unicode::Iterator( src ) {}
        uchar_t Next();
};

class Writer : public Unicode::Writer
{
	public:
		void Append( uchar_t ch );
};

} // namespace UTF8


namespace UTF16 {

class Iterator : public Unicode::Iterator
{
    public:
        Iterator( std::string src ) : Unicode::Iterator( src ) {}
        uchar_t Next();
    protected:
        virtual unsigned int Next16Bits() = 0;
};

class Writer : public Unicode::Writer
{
	public:
    	void Append( uchar_t ch );
	protected:
    	virtual void Next16Bits( unsigned int val ) = 0;
};

} // namespace UTF16

namespace UTF16BE {

class Iterator : public UTF16::Iterator
{
    public:
        Iterator( std::string src ) : UTF16::Iterator( src ) {}
    protected:
        unsigned int Next16Bits();
};

class Writer : public UTF16::Writer
{
	protected:
	    void Next16Bits( unsigned int val );
};

} // namespace UTF16BE

namespace UTF16LE {

class Iterator : public UTF16::Iterator
{
    public:
        Iterator( std::string src ) : UTF16::Iterator( src ) {}
    protected:
        unsigned int Next16Bits();
};

class Writer : public UTF16::Writer
{
	protected:
	    void Next16Bits( unsigned int val );
};

} // namespace UTF16LE

#endif //unicode_h
