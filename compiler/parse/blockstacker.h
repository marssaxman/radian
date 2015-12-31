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


#ifndef blockstacker_h
#define blockstacker_h

#include <vector>
#include "ast.h"
#include "sequence.h"

class BlockStacker : public Iterator<AST::Statement*>
{
    public:
        BlockStacker(
                Iterator<AST::Statement*> &input,
                Reporter &log,
                std::string filepath );
        virtual ~BlockStacker();
        AST::Statement *Current() const { return _current; }
        bool Next();
    protected:
        bool StackContains( const Token &name );
		void CheckIndentation();
        Iterator<AST::Statement*> &_input;
		Reporter &_errors;
        std::vector<Token> _stack;
        AST::Statement *_current;
        SourceLocation _fileLoc;
};

#endif  //blockstacker_h
