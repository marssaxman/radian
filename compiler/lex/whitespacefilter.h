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

#ifndef lex_whitespacefilter_h
#define lex_whitespacefilter_h

#include "lex/token.h"
#include "utility/sequence.h"

class WhitespaceFilter : public Iterator<Token>
{
    public:
        WhitespaceFilter(Iterator<Token> &input);
        bool Next();
        Token Current() const;
        bool Done() const;
    protected:
        Iterator<Token> &_input;
};

#endif //whitespacefilter_h
