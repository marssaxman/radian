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

#ifndef lex_scanner_h
#define lex_scanner_h

#include <string>
#include "unicode/unicode.h"
#include "lex/token.h"
#include "utility/sequence.h"

class Scanner : public Iterator<Token>
{
	public:
		Scanner( std::string source, std::string filepath );
		virtual ~Scanner();
		bool Next();
		bool Done() const;
		Token Current() const;

	protected:
		void Scan();
		void ScanComment();
		void ScanWhitespace();
		void ScanNumber( uchar_t ch );
		void ScanHex();
		void ScanOct();
		void ScanBin();
		void ScanString( uchar_t ch );
		void ScanIdentifier( uchar_t ch );
		void ScanOperator( uchar_t ch );
		void ScanEOL( uchar_t ch );
		void ScanSymbol( uchar_t ch );
		void ScanError();

		bool IsIdentStart( uchar_t ch );
		bool IsIdentContinue( uchar_t ch );
		bool IsIdentStop( uchar_t ch );
		bool IsDigit( uchar_t ch );
		bool IsHexDigit( uchar_t ch );
		bool IsWhitespace( uchar_t ch );

		uchar_t StringChar( unsigned int chars );
		unsigned int HexDigitVal( uchar_t ch );

        std::string _filePath;
		Unicode::Iterator *_source;
		bool _done;

		UTF8::Writer _tokenText;
		unsigned int _tokenStart;
		unsigned int _tokenEnd;
		Token::Type::Enum _typeHint;

		unsigned int _lineIndex;
		int _lineStart;
		bool _flipLineNext;
};

#endif //scanner_h
