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



#ifndef terminal_h
#define terminal_h

#include "numtostr.h"
#include "expression.h"

namespace AST {

class Identifier : public Expression
{
    public:
        Identifier( std::string tk, const SourceLocation &loc );
        std::string Value() const { return _identifier; }
        Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
                { return it->SemGen( *this ); }
        virtual bool IsAnIdentifier() const { return true; }
        virtual const Identifier *AsIdentifier() const { return this; }
        std::string ToString() const { return _identifier; }
		void CollectSyncs( std::queue<const class Sync*> *list ) const {}
    private:
        std::string _identifier;
};

class Literal : public Expression
{
	public:
		Literal( const Token &tk ) : Expression( tk.Location() ), _token(tk) {}
		void CollectSyncs( std::queue<const class Sync*> *list ) const {}
	protected:
		Token _token;
};

class BooleanLiteral : public Literal
{
	public:
		BooleanLiteral( const Token &tk ) : Literal( tk ) {}
		bool Value() const { return _token.BooleanValue(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return Value() ? "true " : "false "; }
};

class Dummy : public Literal
{
	public:
		Dummy( const Token &tk ) : Literal( tk ) {}
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "nil "; }
};

class NumberLiteral : public Literal
{
	protected:
		NumberLiteral( const Token &tk ) : Literal( tk ) {}

	public:
		// Subclasses must override this to provide a representation for their
		// values.  All representations must be normalized into decimal form.
		// For example, the hex literal 0x1234 must return the value "4660".
		virtual std::string Value() const = 0;

		std::string ToString() const { return Value() + " "; }
};

class IntegerLiteral : public NumberLiteral
{
	public:
		IntegerLiteral( const Token &tk ) : NumberLiteral( tk ) {}

		// Integers are stored as a basic string already, so no conversion is
		// required
		virtual std::string Value() const { return _token.Value(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
};

class RealLiteral : public NumberLiteral
{
	public:
		RealLiteral( const Token &tk ) : NumberLiteral( tk ) {}

		// Reals are stored as a basic string already, so no conversion is
		// required
		virtual std::string Value() const { return _token.Value(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
};

// TODO: Because Radian is meant to allow for arbitrary precision numbers, we
// should not be using C math routines for doing numeric conversions (due to
// their limitations). However, since our backend is currently C, the
// limitations are currently in place already!  Whenever we get rid of the C
// backend, we should look at refactoring the hex, oct and bin literal classes
// to using a numeric library (like GMP) to perform the conversion without
// losing precision.
class HexLiteral : public IntegerLiteral
{
	public:
		HexLiteral( const Token &tk ) : IntegerLiteral( tk ) {}
		virtual std::string Value() const;
};

class OctLiteral : public IntegerLiteral
{
	public:
		OctLiteral( const Token &tk ) : IntegerLiteral( tk ) {}
		virtual std::string Value() const;
};

class BinLiteral : public IntegerLiteral
{
	public:
		BinLiteral( const Token &tk ) : IntegerLiteral( tk ) {}
		virtual std::string Value() const;
};

class FloatLiteral : public NumberLiteral
{
	public:
		FloatLiteral( const Token &tk ) : NumberLiteral( tk ) {}
		virtual std::string Value() const { return _token.Value(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
};

class StringLiteral : public Literal
{
	public:
		StringLiteral( const Token &tk ) : Literal( tk ) {}
		std::string Value() const { return _token.Value(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return "\"" + Value() + "\""; }
};

class SymbolLiteral : public Literal
{
	public:
		SymbolLiteral( const Token &tk ) : Literal( tk ) {}
		std::string Value() const { return _token.Value(); }
		virtual Flowgraph::Node *SemGen( ExpressionAnalyzer *it ) const
				{ return it->SemGen( *this ); }
		std::string ToString() const { return ":" + Value(); }
};

} // namespace AST

#endif // terminal_h
