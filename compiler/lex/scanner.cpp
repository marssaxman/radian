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


#include <assert.h>
#include "scanner.h"
#include "unicode.h"
#include "casefolding.h"
#include "charclasses.h"

using namespace std;

Scanner::Scanner( string source, string filepath ) :
	_filePath(filepath),
	_source(NULL),
	_done(false),
	_tokenStart(0),
	_tokenEnd(0),
	_typeHint( Token::Type::Error ),
	_lineIndex(0),
	_lineStart(0),
	_flipLineNext(false)
{
	_source = Unicode::Iterator::GuessEncoding( source );
	// Many text files begin with the byte-order-mark character, U+FEFF. Some
	// Windows text editors like to insert this even in UTF-8 files, which have
	// no byte order, probably to help distinguish them from files using legacy
	// encodings. We don't really care why the file has a BOM, but it is
	// properly considered more of a header than an actual part of the text
	// data, so we will skip it when we find it.
	unsigned int startPos = _source->Position();
	if (0xFEFF != _source->Next()) {
		_source->MoveTo( startPos );
	}
}

Scanner::~Scanner()
{
	delete _source;
}

bool Scanner::Next()
{
	// If we have yet to reach the end of input, scan the next token.
	if (!_source->Done()) {
		Scan();
	} else {
		_done = true;
	}
	return !_done;
}

bool Scanner::Done() const
{
	return _done;
}

// Scanner::Scan
//
// Get the next token. We'll use the first character to identify the token type;
// all tokens can be unambiguously identified by their first character.
// (Reserved words are a subtype of identifiers; that is a matter for the
// lexical analyzer, not the scanner.) Once we have figured out which class of
// token we are looking at, one of the specific token scanners will take over.
// Each token scanner is responsible for setting the mTypeHint we will pass to
// the Token class, and for filling out the mTokenSource string if the content
// of the token is meaningful.
//
void Scanner::Scan()
{
	assert( !_source->Done() );
	// We think of EOL tokens as living on the end of a logical line. We must
	// wait to increment the line count until we start processing the *next*
	// token, following the EOL. We can tell that the previous token must have
	// been an EOL whenever the flag mFlipLineNext is set. We must also keep
	// track of the beginning of each line, so that we can give tokens
	// line-relative source locations.
	if (_flipLineNext)
	{
		_lineIndex++;
		_lineStart = _source->Position();
		_flipLineNext = false;
	}

	// Get the first character of the new token and use it to identify an
	// appropriate sub-scanner. The sub-scanner will grab the rest of the token
	// chars, if any, and fill out the token member data.
	_tokenStart = _source->Position();
	uchar_t ch = _source->Next();
	_tokenEnd = _source->Position();
	_tokenText.Reset();
	if (ch == '#') {
		ScanComment();
	} else if (IsIdentStart( ch )) {
		ScanIdentifier( ch );
	} else if (IsDigit( ch )) {
		ScanNumber( ch );
	} else if (ch == '"' || ch == '\'') {
		ScanString( ch );
	} else if (ch == '\r' || ch == '\n') {
		ScanEOL( ch );
	} else if (IsWhitespace( ch )) {
		ScanWhitespace();
	} else if (':' == ch) {
		ScanSymbol( ch );
	} else if (ch == 0xFFFD ||  ch == '\0') {
		ScanError();
	} else {
		ScanOperator( ch );
	}
	assert(_tokenEnd > _tokenStart);
}

Token Scanner::Current() const
{
	SourceLocation loc(
			_filePath,
			_lineIndex,
			_tokenStart - _lineStart,
			_tokenEnd - _lineStart );
	Token out( _tokenText.Value(), _typeHint, loc);
	return out;
}

// ScanError
//
// If the Unicode input stream discovers malformed characters, or we attempt to
// read off the end of the buffer, it will return U+FFFD, 'REPLACEMENT
// CHARACTER'. We treat this as an "error" token, which will not match any
// pattern.
//
void Scanner::ScanError()
{
	_typeHint = Token::Type::Error;
	_tokenEnd = _source->Position();
}

// ScanEOL
//
// End-of-line conventions differ between operating systems. We accept the
// three common conventions: CR (\r, 0xD), LF (\n, 0xA), and CR LF. I thought
// briefly about forcing people to standardize on a single sensible convention,
// but this is really too stupid an issue to waste human time on. We'll simply
// accept whatever we get. You could even mix conventions within an input
// stream, though that would be dumb.
//
void Scanner::ScanEOL( uchar_t ch )
{
	_typeHint = Token::Type::EOL;
	if (ch == '\r') {
		unsigned int backup = _source->Position();
		if (_source->Next() != '\n') {
			_source->MoveTo( backup );
		}
	} else assert( ch == '\n' );
	_tokenEnd = _source->Position();
    _flipLineNext = true;
}

// Scanner::ScanComment
//
// Comments begin with the comment character and continue to the end of the
// line. This is the traditional comment syntax; it does mean that you can
// stack comments onto statement lines, though this practice is inelegant and
// not recommended. Better practice is to write comments in separate groups.
//
void Scanner::ScanComment()
{
	_typeHint = Token::Type::Comment;
	while (!_source->Done()) {
		unsigned int ch = _source->Next();
		if (ch != '\r' && ch != '\n') {
			_tokenEnd = _source->Position();
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanWhitespace
//
// Whitespace in Radian consists of Unicode whitespace, minus linebreaks and
// tabs. Sensible people, it need hardly be said, use tabs for indentation and
// spaces for spacing.
//
void Scanner::ScanWhitespace()
{
	_typeHint = Token::Type::Whitespace;
	while (!_source->Done()) {
		if (IsWhitespace( _source->Next() )) {
			_tokenEnd = _source->Position();
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanHex
//
// Assumes we've already processed the 0x and are now going to process a
// string of hex digits 0-9A-Fa-f+
//
void Scanner::ScanHex()
{
	_typeHint = Token::Type::Hex;
	_tokenText.Reset();

	while (!_source->Done()) {
		uchar_t ch = _source->Next();
		if (IsHexDigit( ch )) {
			_tokenText.Append( ch );
			_tokenEnd = _source->Position();
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanOct
//
// Assumes we've already processed the 0o and are now going to process a
// string of octal digits 0-7+
//
void Scanner::ScanOct()
{
	_typeHint = Token::Type::Oct;
	_tokenText.Reset();

	while (!_source->Done()) {
		uchar_t ch = _source->Next();
		if (ch >= '0' && ch <= '7') {
			_tokenText.Append( ch );
			_tokenEnd = _source->Position();
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanBin
//
// Assumes we've already processed the 0b and are now going to process a
// string of binary digits 0-1+
//
void Scanner::ScanBin()
{
	_typeHint = Token::Type::Bin;
	_tokenText.Reset();

	while (!_source->Done()) {
		uchar_t ch = _source->Next();
		if (ch == '0' || ch == '1') {
			_tokenText.Append( ch );
			_tokenEnd = _source->Position();
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanNumber
//
// Radian has several types of numeric literals, but all are semantically
// considered to be a "number."  However, because we want to be able to do
// further processing of numbers for special handling, we need to differentiate
// between the various numeric formats. The formats we support are:
//
//	{1-9][0-9]* | 0                 Integer
//	[0-9]+.[0-9]+[f|F]              Real
//	0x[0-9A-Fa-f]+                  Hex
//	0o[0-7]+                        Oct
//	0b[0-1]+                        Bin
//
// TODO: We may want to handle scientific notation, and once we hook up the
// runtime to a numeric toolkit (perhaps GMP), we could also allow users to
// express imaginary numbers too.
//
void Scanner::ScanNumber( uchar_t ch )
{
	// If our string starts with zero, we may need to care
	bool bStartedWithZero = (ch == '0');

	_typeHint = Token::Type::Integer;
	_tokenText.Append( ch );

	bool bProcessedFirstChar = false;
	while (!_source->Done()) {
		ch = _source->Next();
		if (IsDigit( ch )) {
			_tokenText.Append( ch );
			_tokenEnd = _source->Position();
		} else if (ch == '.') {
            _tokenText.Append( ch );
			_tokenEnd = _source->Position();
			_typeHint = Token::Type::Real;
		} else if (bStartedWithZero && !bProcessedFirstChar) {
			// If we're processing the first character after a zero,
			// then we may need to do some special handling.
			if (ch == 'x' || ch == 'X')			ScanHex();
			else if (ch == 'o' || ch == 'O')	ScanOct();
			else if (ch == 'b' || ch == 'B')	ScanBin();
			else                                _source->MoveTo( _tokenEnd );

			// We're done processing
			break;
		} else {
			_source->MoveTo( _tokenEnd );
			break;
		}
		bProcessedFirstChar = true;
	}

	// A real token may end with an optional 'f' suffix, indicating that it
	// represents an approximate floating-point quantity rather than an exact
	// rational quantity.
	if (!_source->Done()) {
		ch = _source->Next();
		if (ch == 'f' || ch == 'F') {
			_tokenText.Append ( ch );
			_tokenEnd = _source->Position();
			_typeHint = Token::Type::Float;
		} else {
			_source->MoveTo( _tokenEnd );
		}
	}

	// We have one special case to check for -- if we processed an integer
	// token that started with a zero, but was multiple characters long, then
	// we didn't really get an integer -- we got some bogus literal.
	if (_typeHint == Token::Type::Integer &&
		bStartedWithZero && bProcessedFirstChar) {
			_typeHint = Token::Type::Unknown;
	}
}

// Scanner::ScanString
//
// String literals begin with a quote mark, proceed with any characters which
// are neither EOL nor quote, and end with a quote. The backslash character
// functions as an escape, according to the following grammar:
//	\\	=>	U+005C backslash
//	\'	=> 	U+0027 apostrophe
//	\"	=>	U+0022 quote
//	\a	=>	U+0007 bell
//	\b	=>	U+0008 backspace
//	\f	=>	U+000C form feed
//	\n	=>	U+000A newline
//	\r	=>	U+000D carriage return
//	\t	=>	U+0009 horizontal tab
//	\v	=>	U+000B vertical tab
//	\xXX	=>	One-byte character, specified by two hex digits
//	\uXXXX	=>	BMP character, specified by four hex digits
//	\UXXXXXX =>	any character, specified by six hex digits
// Any other escape character is an error. Escapes are case sensitive. Octal
// omitted on purpose.
//
void Scanner::ScanString( uchar_t ch )
{
	_typeHint = Token::Type::String;
	assert( ch == '"' || ch == '\'' );

	// We want to match the start and end delimeters of the string properly
	uchar_t endSequence = (ch == '"' ? '"' : '\'');

	while (!_source->Done()) {
		ch = _source->Next();
		if (ch == endSequence) {
			_tokenEnd = _source->Position();
			break;
		}
		if (ch == '\n' || ch == '\r') {
			_source->MoveTo( _tokenEnd );
			ScanError();
			break;
		}
		if ('\\' == ch && !_source->Done()) {
			ch = _source->Next();
			switch (ch) {
				case '\\': ch = '\\'; break;
				case '\'': ch = '\''; break;
				case '\"': ch = '\"'; break;
				case 'a': ch = '\a'; break;
				case 'b': ch = '\b'; break;
				case 'f': ch = '\f'; break;
				case 'n': ch = '\n'; break;
				case 'r': ch = '\r'; break;
				case 't': ch = '\t'; break;
				case 'v': ch = '\v'; break;
				case 'x': ch = StringChar( 2 ); break;
				case 'u': ch = StringChar( 4 ); break;
				case 'U': ch = StringChar( 6 ); break;
				default: ScanError(); return;
			}
		}
		_tokenText.Append( ch );
	}
}

// Scanner::StringChar
//
// Get a character specified in hex digits. Our parameter tells us how many
// digits to expect.
//
uchar_t Scanner::StringChar( unsigned int digits )
{
	uchar_t out = 0;
	while (digits > 0 && !_source->Done()) {
		uchar_t ch = _source->Next();
		if (!IsHexDigit( ch )) {
			ScanError();
			return 0xFFFD;
		}
		out = (out << 4) | HexDigitVal( ch );
		digits--;
	}
	return out;
}

// Scanner::ScanIdentifier
//
// Identifiers begin with an IdentStart character and continue for each
// succeeding IdentContinue character, optionally ending with an IdentStop.
// Radian is a case-insensitive language, so we apply case-folding as we build
// the token text. This allows the rest of the compiler to almost entirely
// ignore case issues, since almost all symbols begin life as identifier tokens.
// (One must still be careful to define all internal strings in case-folded
// form, which basically just means they need to be defined in lowercase.)
//
void Scanner::ScanIdentifier( uchar_t ch )
{
	_typeHint = Token::Type::Identifier;
	UTF8::FoldCase( ch, _tokenText );
	while (!_source->Done()) {
		ch = _source->Next();
		bool done = !IsIdentContinue( ch );
		bool accept = !done || IsIdentStop( ch );
		if (accept) {
			UTF8::FoldCase( ch, _tokenText );
			_tokenEnd = _source->Position();
		}
		if (done) {
			_source->MoveTo( _tokenEnd );
			break;
		}
	}
}

// Scanner::ScanSymbol
//
// Symbols begin with a colon and continue with an identifier. If the next char
// is an IdentStart, we will continue assembling a case-folded symbol token
// until we run out of IdentContinue characters; otherwise, the token is just a
// colon. The token text is the identifier text and does not include the colon.
//
void Scanner::ScanSymbol( uchar_t ch )
{
	_typeHint = Token::Type::Unknown;
	_tokenEnd = _source->Position();
	if (_source->Done()) return;

	// Take a peek at the next char. If it is an identifier char, then this is
	// a symbol. The rest of the pattern is identical to that of an identifier,
	// so we'll just scan it as an identifier. If not, then this is a one-
	// character colon token, not a symbol.
	uchar_t nextch = _source->Next();
	if (IsIdentStart( nextch )) {
		ScanIdentifier( nextch );
		_typeHint = Token::Type::Symbol;
	}
	else {
		_tokenText.Append( ch );
		_source->MoveTo( _tokenEnd );
	}
}

// ScanOperator
//
// Pick off an operator token: this is any punctuation-mark type token. Most
// operator tokens are single characters, but we have some two-character
// operators as well. We'll check to see if we can match a two-char token; if
// we can't, we'll fall back and return the current single character as its own
// token. If this proved to be a performance bottleneck, we could first check
// to see whether the current character is one of the small handful that could
// potentially begin a two-char token.
//
void Scanner::ScanOperator( uchar_t ch )
{
	_typeHint = Token::Type::Unknown;
	_tokenText.Append( ch );
	int backup = _source->Position();
	if (!_source->Done()) {
		_tokenText.Append( _source->Next() );
		if (!Token::MatchOperator( _tokenText.Value() )) {
			_tokenText.Reset();
			_tokenText.Append( ch );
			_source->MoveTo( backup );
		}
	}
	_tokenEnd = _source->Position();
}

bool Scanner::IsIdentStart( uchar_t ch )
{
	if (ch == '_') return true;
	return Unicode::XID_Start( ch );
}

bool Scanner::IsIdentContinue( uchar_t ch )
{
	return Unicode::XID_Continue( ch );
}

bool Scanner::IsIdentStop( uchar_t ch )
{
	if (ch == '=') return true;
	return false;
}

bool Scanner::IsDigit( uchar_t ch )
{
	return ch >= '0' && ch <= '9';
}

bool Scanner::IsHexDigit( uchar_t ch )
{
	return (ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

// Scanner::IsWhitespace
//
// "Whitespace" refers to runs of whitespace characters which have no meaning,
// aside from their role as token separators. We do not include LF, CR, or HT,
// which have structural significance, but aside from those we accept all
// Unicode whitespace characters.
//
bool Scanner::IsWhitespace( uchar_t ch )
{
	if (ch == 0x000D || ch == 0x000A || ch == 0x0009) return false;
	return Unicode::Pattern_White_Space( ch );
}

// Scanner::HexDigitVal( uchar_t ch )
//
// Get the numeric value of this hex digit. Only meaningful on chars which
// actually are hex digits. Use IsHexDigit() to find out which chars count; we
// only support the ASCII hex chars.
//
unsigned int Scanner::HexDigitVal( uchar_t ch )
{
	if (ch >= '0' && ch <= '9') {
		return ch - '0' + 0;
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 0xA;
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 0xA;
	} else {
		assert( !IsHexDigit( ch ) );
		assert( false );
		return 0;
	}
}
