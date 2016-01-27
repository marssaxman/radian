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

#include <map>
#include <assert.h>
#include "lex/token.h"

using namespace std;

map<string, Token::Type::Enum> sTokenMap;
bool sTokenMapReady;

Token::Token(string text, Type::Enum hint, const SourceLocation &loc) :
    _text(text),
    _type(Token::Type::Unknown),
	_location(loc)
{
	Categorize(hint);
}

string Token::Dump() const
{
	string out = "{";
	out += "text: \"" + _text + "\", ";
	out += "location: " + _location.Dump();
	out += "}";
	return out;
}

void Token::Categorize(Token::Type::Enum hint)
{
	SetUpTokenMap();
	// The scanner does some preliminary categorization, expressed through
	// the type hint. We will break the type down further.
	switch (hint) {
		case Token::Type::Identifier:
		case Token::Type::Unknown: {
			map<string, Token::Type::Enum>::iterator found =
					sTokenMap.find( _text );
			if (found != sTokenMap.end()) {
				_type = found->second;
			} else {
				_type = hint;
			}
		} break;
		case Token::Type::Error:
		case Token::Type::Comment:
		case Token::Type::Whitespace:
		case Token::Type::Indent:
		case Token::Type::EOL:
		case Token::Type::Symbol:
		case Token::Type::Integer:
		case Token::Type::Real:
		case Token::Type::Float:
		case Token::Type::Hex:
		case Token::Type::Oct:
		case Token::Type::Bin:
		case Token::Type::String: {
			_type = hint;
		} break;
		case Token::Type::KeyIf:
		case Token::Type::KeyWhile: {
			// These two keywords are handled differently because they can be
			// used as block-end identifiers. The compiler will internally
			// construct a token using the relevant token ID, and that will
			// come through this path rather than the Type::Identifier path.
			// If you create any new block types which use reserved words for
			// their end-block statements, you'll need to add those keyword IDs
			// to the case list above.
			map<string, Token::Type::Enum>::iterator found =
					sTokenMap.find( _text );
			assert( found != sTokenMap.end() );
			assert( hint == found->second );
			_type = hint;
		} break;
		default: assert(false);
	}
}

Token::Token( const Token &other ) :
    _text(other._text),
    _type(other._type),
	_location(other._location)
{
}

Token::Token() :
	_text( "" ),
	_type( Token::Type::Unknown )
{
}

void Token::SetUpTokenMap(void)
{
	if (!sTokenMapReady) {
		// Identifier and keyword matching is not case sensitive in Radian.
		// The scanner will normalize identifiers using Unicode
		// locale-independent case-folding. For ASCII characters, this just
		// means that they are lowercase. All keywords must therefore be
		// defined here in lowercase, or they  will not match actual identifier
		// strings. This list is kept in the same order as the type enum
		// because it's prettier that way, not for any functional reason.
		sTokenMap[ "and" ] = Token::Type::KeyAnd;
		sTokenMap[ "as" ] = Token::Type::KeyAs;
		sTokenMap[ "assert" ] = Token::Type::KeyAssert;
		sTokenMap[ "capture" ] = Token::Type::KeyCapture;
		sTokenMap[ "debug_trace" ] = Token::Type::KeyDebug_Trace;
		sTokenMap[ "def" ] = Token::Type::KeyDef;
		sTokenMap[ "each" ] = Token::Type::KeyEach;
		sTokenMap[ "else" ] = Token::Type::KeyElse;
		sTokenMap[ "end" ] = Token::Type::KeyEnd;
		sTokenMap[ "false" ] = Token::Type::KeyFalse;
		sTokenMap[ "for" ] = Token::Type::KeyFor;
		sTokenMap[ "from" ] = Token::Type::KeyFrom;
		sTokenMap[ "function" ] = Token::Type::KeyFunction;
		sTokenMap[ "has" ] = Token::Type::KeyHas;
		sTokenMap[ "import" ] = Token::Type::KeyImport;
		sTokenMap[ "if" ] = Token::Type::KeyIf;
		sTokenMap[ "in" ] = Token::Type::KeyIn;
		sTokenMap[ "invoke" ] = Token::Type::KeyInvoke;
		sTokenMap[ "method" ] = Token::Type::KeyMethod;
		sTokenMap[ "mod" ] = Token::Type::KeyMod;
		sTokenMap[ "not" ] = Token::Type::KeyNot;
		sTokenMap[ "object" ] = Token::Type::KeyObject;
		sTokenMap[ "or" ] = Token::Type::KeyOr;
		sTokenMap[ "sync" ] = Token::Type::KeySync;
		sTokenMap[ "then" ] = Token::Type::KeyThen;
		sTokenMap[ "throw" ] = Token::Type::KeyThrow;
		sTokenMap[ "true" ] = Token::Type::KeyTrue;
		sTokenMap[ "var" ] = Token::Type::KeyVar;
		sTokenMap[ "where" ] = Token::Type::KeyWhere;
		sTokenMap[ "while" ] = Token::Type::KeyWhile;
		sTokenMap[ "xor" ] = Token::Type::KeyXor;
		sTokenMap[ "yield" ] = Token::Type::KeyYield;

		sTokenMap[ "\t" ] = Token::Type::Indent;
		sTokenMap[ "(" ] = Token::Type::ParenL;
		sTokenMap[ ")" ] = Token::Type::ParenR;
		sTokenMap[ "[" ] = Token::Type::BracketL;
		sTokenMap[ "]" ] = Token::Type::BracketR;
		sTokenMap[ "{" ] = Token::Type::BraceL;
		sTokenMap[ "}" ] = Token::Type::BraceR;
		sTokenMap[ "." ] = Token::Type::Period;
		sTokenMap[ "\xE2\x86\x90" ] =
				Token::Type::LeftArrow; // U+2190 'LEFTWARDS ARROW'
		sTokenMap[ "\xE2\x86\x92" ] =
				Token::Type::RightArrow; // U+2192 'RIGHTWARDS ARROW'
		sTokenMap[ "<" ] = Token::Type::OpLT;
		sTokenMap[ ">" ] = Token::Type::OpGT;
		sTokenMap[ "=" ] = Token::Type::OpEQ;
		sTokenMap[ "\xE2\x89\xA0" ] =
				Token::Type::OpNE; // U+2260 'NOT EQUAL TO'
		sTokenMap[ "\xE2\x89\xA4" ] =
				Token::Type::OpLE; // U+2264 'LESS-THAN OR EQUAL TO'
		sTokenMap[ "\xE2\x89\xA5" ] =
				Token::Type::OpGE; // U+2265 'GREATER-THAN OR EQUAL TO'
		sTokenMap[ "+" ] = Token::Type::Plus;
		sTokenMap[ "-" ] = Token::Type::Minus;
		sTokenMap[ "\xC3\x97" ] =
			Token::Type::Multiplication; // U+00D7 'MULTIPLICATION SIGN'
		sTokenMap[ "\xC3\xB7" ] =
			Token::Type::Division;  // U+00F7 'DIVISION SIGN'
		sTokenMap[ "&" ] = Token::Type::Ampersand;
		sTokenMap[ "," ] = Token::Type::Comma;
		sTokenMap[ ":" ] = Token::Type::Colon;
		sTokenMap[ "<<" ] = Token::Type::ShiftLeft;
		sTokenMap[ ">>" ] = Token::Type::ShiftRight;
		sTokenMap[ "*" ] = Token::Type::Asterisk;
		sTokenMap[ "**" ] = Token::Type::DoubleAsterisk;
		sTokenMap[ "/" ] = Token::Type::Solidus;
		sTokenMap[ "\xE2\x87\x92" ] =
			Token::Type::RightDoubleArrow; // U+21D2 'RIGHTWARDS DOUBLE ARROW'
		sTokenMap[ "\xE2\x98\xA0" ] =
			Token::Type::Poison; // U+2620 'SKULL AND CROSSBONES'

		// Some alternate ASCII spellings, for tokens that are hard to type,
		// especially on operating systems with inferior input systems:
		sTokenMap[ "<=" ] = Token::Type::OpLE;
		sTokenMap[ ">=" ] = Token::Type::OpGE;
		sTokenMap[ "!=" ] = Token::Type::OpNE;
		sTokenMap[ "->" ] = Token::Type::RightArrow;
		sTokenMap[ "<-" ] = Token::Type::LeftArrow;
		sTokenMap[ "=>" ] = Token::Type::RightDoubleArrow;

		// consider adding python string interpolation operator %
		// consider adding bitwise not, or, xor, and operators: maybe call them
		//  bitnot, bitor, bitxor, bitand since the relevant punctuation marks
		//	are not necessarily available
		// consider adding ruby .. as a range operator (alias for number.range)

		// unused ASCII punctuation: @ $ % ; \ ` ~ ^ |
		// unused as solo chars: _ ? !

		sTokenMapReady = true;
	}
}

bool Token::MatchOperator(string sym)
{
	SetUpTokenMap();
	return sTokenMap.count(sym) > 0;
}

bool Token::BooleanValue(void) const
{
	switch (_type) {
		case Type::KeyTrue: return true;
		case Type::KeyFalse: return false;
		default: assert( false ); return false;
	}
}
