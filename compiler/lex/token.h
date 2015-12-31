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


#ifndef token_h
#define token_h

#include <string>
#include "sourcelocation.h"

class Token
{
	public:
		struct Type { enum Enum
		{
			Unknown,
			Error,
			Comment,
			Whitespace,
			Indent,
			EOL,
			Identifier,
			Symbol,
			Integer,
			Real,
			Float,
			Hex,
			Oct,
			Bin,
			String,
			ParenL,
			ParenR,
			BracketL,
			BracketR,
			BraceL,
			BraceR,
			Period,
			LeftArrow,
			RightArrow,
			OpLT,
			OpGT,
			OpEQ,
			OpNE,
			OpLE,
			OpGE,
			Plus,
			Minus,
			Multiplication,
			Division,
			Ampersand,
			Comma,
			Colon,
			Bar,
			ShiftRight,
			ShiftLeft,
			Asterisk,
			DoubleAsterisk,
			Solidus,
			RightDoubleArrow,
			Poison,
			KeyAnd,
			KeyAs,
			KeyAssert,
			KeyCapture,
			KeyDebug_Trace,
			KeyDef,
			KeyEach,
			KeyElse,
			KeyEnd,
			KeyFalse,
			KeyFor,
			KeyFrom,
			KeyFunction,
			KeyHas,
			KeyIf,
			KeyImport,
			KeyIn,
			KeyInvoke,
			KeyMethod,
			KeyMod,
			KeyNot,
			KeyObject,
			KeyOr,
			KeySync,
			KeyThen,
			KeyThrow,
			KeyTrue,
			KeyVar,
			KeyWhere,
			KeyWhile,
			KeyXor,
			KeyYield,
		};};

		Token();
		Token(std::string text, Type::Enum hint, const SourceLocation &loc);
		Token(const Token &other);
		std::string Dump() const;
		static bool MatchOperator(std::string sym);
		std::string Value(void) const { return _text; }
		bool BooleanValue(void) const;
		Type::Enum TokenType(void) const { return _type; }
		SourceLocation Location(void) const { return _location; }

	protected:
		void Categorize(Type::Enum hint);
		static void SetUpTokenMap(void);  
		std::string _text;
		Type::Enum _type;
		SourceLocation _location;
};

#endif //token_h

