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

#include "parse/parser.h"
#include "parse/statementparser.h"

using namespace Parser;

Engine::Engine( Iterator<Token> &input, Reporter &log ):
	_input(input),
	_errors(log),
	_current(NULL)
{
}

bool Engine::Next()
{
	if (_current) {
		delete _current;
		_current = NULL;
	}
	// This parser turns a stream of tokens into a stream of radian statements.
	// Statements consist of one or more complete input lines. Almost all
	// statements can be identified by their first token.
	Parser::Statement engine( _input, _errors );
	_current = engine.Result();
	return _current != NULL;
}
