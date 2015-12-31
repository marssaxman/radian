// Copyright 2012 Mars Saxman.
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

#include "closure.h"

using namespace Semantics;
using namespace Flowgraph;

Closure::Closure( Pool &pool, Scope *context ):
	Layer(pool, context),
	_captureList(pool.Nil()),
	_captureCount(0),
	_paramCount(0)
{
}

Node *Closure::CreateLocalReference( Node *varSym, Node *value )
{
	if (value->IsContextIndependent()) return value;
	_captureList = _pool.ArgsAppend( _captureList, value );
	return _pool.Slot( _captureCount++ );
}

void Closure::RebindInContext(
		Node *sym, Node *value, const SourceLocation &loc )
{
	// Functions cannot rebind context variables, because a function can be
	// called frum any number of different sites and thus cannot guarantee that
	// the context variables will even exist at the time of invocation.
	ReportError( Error::Type::ContextVarRedefinition, loc );
}

Node *Closure::Capture( Node *resultValue )
{
	// Create the output function for this closure. We expect resultValue to be
	// some expression. We will wrap this expression in a function node with
	// the previously-defined arity, then attach a list of all the context
	// values we need to retain in order to populate the closure instance's
	// slots. There are too many different applications of the word "capture"
	// in this codebase.
	if (IsSegmented()) {
		resultValue = PackageSegmentedResult( resultValue );
	}
	std::string name = FullyQualifiedName();
	Node *function = _pool.Function( resultValue, _paramCount, name );
	if (_captureCount > 0) {
		function = _pool.CaptureN( function, _captureList );
	}
	return function;
}

void Closure::DefineParameters(
		const AST::Expression *paramExp,
		const SourceLocation &loc )
{
	if (Context().NeedsImplicitSelf()) {
		// If this function is being defined directly inside a scope which acts
		// as a dispatcher for its members, we must define the implicit "self"
		// parameter which refers to the containing scope. We must do this
		// before registering the explicit parameters, since the caller will
		// always pass the self-value in as the first argument.
		DefineOneParameter(
			Context().ImplicitSelfName(),
			SelfParameterType(),
			loc );
	}
	ProcessParamList( paramExp );
}

void Closure::DefineOneParameter(
		Node *symbol, Symbol::Type::Enum kind, const SourceLocation &loc )
{
	Node *val = _pool.Parameter( _paramCount++ );
	Define( symbol, val, kind, loc );
}

void Closure::ProcessParamList( const AST::Expression *paramExp )
{
	// Our parameter expression is either empty, an identifier, or a tuple of
	// identifiers. Register symbols accordingly. 1-tuples are inelegant, so we
	// only use the tuple-lookup definitions when there are multiple parameter
	// definitions.
	if (paramExp) {
		if (paramExp->IsAOpTuple()) {
			const AST::OpTuple *paramList =
					static_cast<const AST::OpTuple*>(paramExp);
			ProcessParamList( paramList->Left() );
			paramExp = paramList->Right();
		}
		if (paramExp->IsAnIdentifier()) {
			const AST::Identifier *ident =
					static_cast<const AST::Identifier*>(paramExp);
			Node *varSym = _pool.Symbol( ident->Value() );
			DefineOneParameter( varSym, Symbol::Type::Var, ident->Location() );
		} else {
			ReportError(
					Error::Type::ParamExpectsIdentifier,
					paramExp->Location() );
		}
	}
}

