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

#include <assert.h>
#include "ast/binop.h"
#include "flowgraph/flowgraph.h"

using namespace AST;
using namespace Flowgraph;

BinOp::BinOp(
    Expression *left,
    Expression *right ):
    Expression( left->Location() + right->Location() ),
    _left( left ),
    _right( right )
{
    assert( left );
    assert( right );
}

BinOp::~BinOp()
{
    delete _left;
    delete _right;
}

std::string BinOp::ToString() const
{
	return _left->ToString() + " " + OpString() + " " + _right->ToString();
}

void BinOp::CollectSyncs( std::queue<const class Sync*> *list ) const
{
	// This is a left-to-right depth first search for Sync nodes.
	_left->CollectSyncs( list );
	_right->CollectSyncs( list );
}


Expression *BinOp::Reassociate()
{
    // I am not satisfied with this system yet.
    // It shouldn't need to make explicit references to BinOp.
    // It should also allow reassociation of unary operators.
	bool reassociate = false;
	if (_left->IsABinOp()) {
		if (Precedence() > _left->Precedence()) {
			reassociate = true;
		} else if (
				Precedence() == _left->Precedence() &&
				Associativity() == Association::Right) {
			assert( Associativity() == _left->Associativity() );
			reassociate = true;
		}
	}
    if (reassociate) {
        BinOp *previousRoot = static_cast<BinOp*>(_left);
        _left = previousRoot->_right;
        previousRoot->_right = this->Reassociate();
        return previousRoot;
    } else {
        return this;
    }
}

bool BinOp::IsBinOpToken( const Token &tk )
{
	// Is this token a symbol for a two-operand operation?
	// The parser uses this to find out whether, given a token, it should try
	// to collect another expression (representing the right operand) or give
	// up parsing the expression. This list must be kept in sync with the list
	// in BinOp::Create. Missing a case here will cause that expression-token
	// to be unrecognized; missing a case there will cause a failed assertion
	// when using the operator.
	switch (tk.TokenType()) {
		case Token::Type::Comma:
		case Token::Type::KeyIf:
		case Token::Type::KeyElse:
		case Token::Type::OpLT:
		case Token::Type::OpGT:
		case Token::Type::OpEQ:
		case Token::Type::OpNE:
		case Token::Type::OpLE:
		case Token::Type::OpGE:
		case Token::Type::Plus:
		case Token::Type::Minus:
		case Token::Type::Multiplication:
		case Token::Type::Division:
		case Token::Type::KeyMod:
		case Token::Type::Asterisk:
		case Token::Type::DoubleAsterisk:
		case Token::Type::Solidus:
		case Token::Type::KeyAnd:
		case Token::Type::KeyOr:
		case Token::Type::KeyXor:
		case Token::Type::KeyHas:
		case Token::Type::KeyAs:
		case Token::Type::Ampersand:
		case Token::Type::ShiftLeft:
		case Token::Type::ShiftRight:
		case Token::Type::RightDoubleArrow:
			return true;
		default:
			return false;
	}
}

BinOp *BinOp::Create( Expression *left, const Token &tk, Expression *right )
{
	// Build a binop node for this operation.
	// The parser supplies us with the operands and the token char.
	// This list of instantiations must be kept in sync with the list of
	// operator tokens in BinOp::IsBinOpToken.
	assert( IsBinOpToken( tk ) );
	using namespace AST;
	switch (tk.TokenType()) {
		case Token::Type::Comma: return new OpTuple( left, right );
		case Token::Type::KeyIf: return new OpIf( left, right );
		case Token::Type::KeyElse: return new OpElse( left, right );
		case Token::Type::OpLT: return new OpLT( left, right );
		case Token::Type::OpGT: return new OpGT( left, right );
		case Token::Type::OpEQ: return new OpEQ( left, right );
		case Token::Type::OpNE: return new OpNE( left, right );
		case Token::Type::OpLE: return new OpLE( left, right );
		case Token::Type::OpGE: return new OpGE( left, right );
		case Token::Type::Plus: return new OpAdd( left, right );
		case Token::Type::Minus: return new OpSubtract( left, right );
		case Token::Type::Asterisk: // asterisk substitutes for multiply
		case Token::Type::Multiplication: return new OpMultiply( left, right );
		case Token::Type::Solidus: // solidus substitutes for divide
		case Token::Type::Division: return new OpDivide( left, right );
		case Token::Type::KeyMod: return new OpModulus( left, right );
		case Token::Type::DoubleAsterisk: return new OpExponent( left, right );
		case Token::Type::KeyAnd: return new OpAnd( left, right );
		case Token::Type::KeyOr: return new OpOr( left, right );
		case Token::Type::KeyXor: return new OpXor( left, right );
		case Token::Type::KeyHas: return new OpHas( left, right );
		case Token::Type::KeyAs: return new OpAssert( left, right );
		case Token::Type::Ampersand: return new OpConcat( left, right );
		case Token::Type::ShiftLeft: return new OpShiftLeft( left, right );
		case Token::Type::ShiftRight: return new OpShiftRight( left, right );
		case Token::Type::RightDoubleArrow: return new OpPair( left, right );
		default: {
			// This method has gotten out of sync with IsBinOpToken.
			// It must be recognizing some token we aren't handling.
			assert( !IsBinOpToken( tk ) );
			return NULL;
		}
	}
}

void OpTuple::UnpackTuple( std::stack<const Expression*> *explist ) const
{
	// Unpack this tuple chain into a stack, such that the leftmost item is
	// the top of the stack. Well, obviously we need to push our right operand,
	// then unpack our left operand. Tail recursive operation in C++! I wonder
	// if gcc is smart enough to make it work.
	explist->push( Right() );
	Left()->UnpackTuple( explist );
}

std::string OpTuple::ToString() const
{
	return Left()->ToString() + ", " + Right()->ToString();
}
