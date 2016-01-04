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

#ifndef parse_parser_h
#define parse_parser_h

#include "utility/sequence.h"
#include "lex/token.h"
#include "ast/ast.h"
#include "parse/parsercore.h"

namespace Parser {

class Engine : public Iterator<AST::Statement*>
{
	public:
		Engine( Iterator<Token> &input, Reporter &log );
		~Engine() { if (_current) delete _current; }
		bool Next();
		AST::Statement *Current() const { return _current; }
	protected:
		Iterator<Token> &_input;
		Reporter &_errors;
		AST::Statement *_current;
};

} // namespace Parser

#endif // parser_h
