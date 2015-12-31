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


#include "blockstacker.h"

BlockStacker::BlockStacker(
		Iterator<AST::Statement*> &input,
		Reporter &log,
		std::string filepath ):
	_input(input),
	_errors(log),
	_current(NULL),
	_fileLoc(SourceLocation::File(filepath))
{
}

BlockStacker::~BlockStacker()
{
	// We sometimes create bogus statement objects in order to sanitize the
	// output stream. We will have at most one of these objects hanging around
	// at a time, since we only yield one statement at a time; if our current
	// statement is not the same as the current statement of our input stream,
	// then we must have ended our process with a bogus object, which it is now
	// our responsibility to delete.
	if (_current && _current != _input.Current()) {
		delete _current;
	}
}

bool BlockStacker::Next()
{
	// If the current statement is a block-end, it must match the top of our
	// block stack. (We never return a series of statements with mismatched
	// blocks.) Since we're about to move on to the next statement, pop the
	// block context, in case the next statement matches our previous block.
	if (_current && _current->IsBlockEnd()) {
		assert( _stack.size() > 0 );
		assert( _current->EndsThisBlock( _stack.back() ) );
		_stack.pop_back();
	}
	
	// Try to get another statement: 
	bool good = false;
	if (!_current || _current == _input.Current()) {
		good = _input.Next();
	} else if (_current->IsBlockEnd()) {
		// When the user has omitted an end block statement, we will synthesize
		// one. We can tell that this has happened when our current statement
		// doesn't match our input's current. In that case, we should not grab
		// a new statement, but return the one the input already has.
		good = _input.Current() != NULL;
		delete _current;
	} else {
		// When the user has supplied an end block statement which doesn't
		// match, we will return a blank line in place of the original
		// statement.
		good = _input.Next();
		delete _current;
	}
	_current = _input.Current();
		
	// If we have a new statement, figure out whether it affects our block
	// stack.
	if (good) {
		assert( _current );
		CheckIndentation();
		if (_current->IsBlockBegin()) {
			_stack.push_back( _current->BlockName() );
		} else if (_current->IsBlockEnd()) {
			// This block-end statement ought to match the top of the
			// declaration stack. If it does not, we should handle this as an
			// error.
			if (_stack.size() == 0) {
				// There is nothing on the stack at all, so this is a spurious
				// End. We will return a blank line instead, with an
				// appropriate error message.
				_current = new AST::BlankLine(
						_current->IndentLevel(), _current->Location() );
				_errors.ReportError(
						Error::Type::UnmatchedEndBlock, _current->Location() );
			} else if (_current->EndsThisBlock( _stack.back() )) {
				// Success! This statement ends the current block.
				// We don't need to do anything special; we can return this
				// statement as-is.
			} else if (StackContains( _current->BlockName() )) {
				// This end statement does not match the current block, but it
				// does match a block further up the stack. We assume that the
				// programmer omitted the End statement for the topmost block.
				// We will synthesize one, with an appropriate error message
				// and location, and return it in place of the actual next
				// instruction.
				_current = new AST::BlockEnd(
						_current->IndentLevel(), _current->Location() );
				_errors.ReportError(
						Error::Type::UnmatchedBeginBlock,
						_stack.back().Location() );
			} else {
				// There are blocks on the stack, but this end statement does
				// not match any of them. We will return a blank line with an
				// error message in place of this block end.
				_current = new AST::BlankLine(
						_current->IndentLevel(), _current->Location() );
				_errors.ReportError(
						Error::Type::UnmatchedEndBlock, _current->Location() );
			}
		}
	} else {
		// The input has no more statements. If we still have an element on the
		// stack, return a fake BlockEnd with an appropriate error. This
		// process will repeat until the block stack empties.
		if (_stack.size() > 0) {
			_current = new AST::BlockEnd( 0, _fileLoc );
			_errors.ReportError(
					Error::Type::UnmatchedBeginBlock, 
					_stack.back().Location() );
			good = true;
		} else {
			assert( !_current );
		}
	}
	return good;
}

bool BlockStacker::StackContains( const Token &name )
{
	std::string nameVal = name.Value();
	for (unsigned int i = 1; i <= _stack.size(); i++) {
		if (_stack[_stack.size() - i].Value() == nameVal) {
			return true;
		}
	}
	return false;
}

// BlockStacker::CheckIndentation
//
// We have pulled a new statement. Its indentation level should match the 
// current nesting level. If it is an ending statement, it should match the
// previous nesting level.
//
void BlockStacker::CheckIndentation()
{
	unsigned int itsLevel = _current->IndentLevel();
	unsigned int targetLevel = _stack.size();
	if (_current->IsBlockEnd() || _current->DelimitsBlock()) {
		if (targetLevel > 0) targetLevel--;
	}
	if (itsLevel < targetLevel) {
		_errors.ReportError(
				Error::Type::InsufficientIndentation, _current->Location() );
	}
	if (itsLevel > targetLevel) {
		_errors.ReportError(
				Error::Type::ExcessiveIndentation, _current->Location() );
	}
}
