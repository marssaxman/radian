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

#ifndef parse_blanklinefilter_h
#define parse_blanklinefilter_h

#include "ast/ast.h"
#include "utility/sequence.h"

class BlankLineFilter : public Iterator<AST::Statement*>
{
	public:
		BlankLineFilter(Iterator<AST::Statement*> &input) : _input(input) {}
		virtual ~BlankLineFilter() {}
		AST::Statement *Current() const { return _input.Current(); }
		bool Next();
	protected:
		Iterator<AST::Statement*> &_input;
};

#endif	//blanklinefilter_h
