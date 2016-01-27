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

#ifndef semantics_scope_h
#define semantics_scope_h

#include <map>
#include "flowgraph/flowgraph.h"
#include "semantics/symbols.h"
#include "semantics/segment.h"
#include "ast/ast.h"

namespace Semantics {

// A scope is a domain in which symbols can be defined and resolved.
class Scope : public Reporter
{
	public:
		Scope( Flowgraph::Pool &pool );
		virtual ~Scope();

		// Each scope has a globally unique name, qualified by its containing
		// scope. We can use this to generate globally unique names for items
		// defined inside a scope.
		virtual std::string FullyQualifiedName() const = 0;

		// Evaluate an expression in this scope, returning a flowgraph node
		// which represents its value.
		Flowgraph::Node *Eval( const AST::Expression *it );
		// Evaluate a statement in this scope, returning the scope to use for
		// the next statement.
		Scope *SemGen( const AST::Statement *it );

		// The primary function of a scope is to map symbols to expression
		// trees, allowing us to define named items and refer to them later.
		// Look up the value associated with some symbol.
		virtual Symbol Resolve( Flowgraph::Node *name );
		// Define a new symbol in the current scope. It's OK to shadow
		// symbols defined in containing scopes.
		virtual void Define(
			Flowgraph::Node *name,
			Flowgraph::Node *value,
			Symbol::Type::Enum kind,
			const SourceLocation &loc );
		// Assign a new value to some existing variable.
		virtual void Assign(
			Flowgraph::Node *name,
			Flowgraph::Node *value,
			const SourceLocation &loc );
		// Shut down the current scope and return a reference to the new
		// context.
		virtual Scope *Exit( const SourceLocation &loc ) = 0;

		// If the scope uses a member dispatcher, function-like blocks will add
		// an implicit "self" to their parameter lists, corresponding to the
		// object value which will be supplied when the function is called.
		virtual bool NeedsImplicitSelf() const { return false; };
		virtual Flowgraph::Node *ImplicitSelfName() const
				{ assert( false ); return _pool.Nil(); }

		// Specialized behaviors for specific scope types: these statements
		// produce errors by default, but can be overridden to work in certain
		// scopes that expect to find them.
		virtual Scope *PartitionElse( const AST::Else &it );

		// One semantic scope may be separated into multiple segments, which
		// can be evaluated sequentially as an asynchronous process. This is
		// the foundation of the async generator/task system.
		void PushSegment(
				Flowgraph::Node *value,
				Segment::Type::Enum type,
				const SourceLocation &loc );

	protected:
		// Every scope needs to know where the pool is.
		Flowgraph::Pool &_pool;

		// Each scope must define its own behavior when it comes to capturing
		// or rebinding symbols defined in outer contexts, since there is no
		// generic mechanism which can apply to all scopes.
		virtual Symbol CaptureFromContext( Flowgraph::Node *name ) = 0;
		virtual void RebindInContext(
				Flowgraph::Node *name,
				Flowgraph::Node *value,
				const SourceLocation &loc ) = 0;

		bool IsSegmented(void) const { return _segments != NULL; }
		bool SegmentsSynchronize(void) const;
		Flowgraph::Node *PackageSegmentedResult( Flowgraph::Node *result );
		void RewriteSegmentCaptures( Flowgraph::NodeMap &reMap );

	private:
		// _symbols contains only those symbols which are active in the current
		// segment. When we flip segments, we will clear this list, but we will
		// maintain the list of symbols that have been defined. When we try to
		// resolve, assign, or define a symbol which is on the definition list,
		// we can retrieve the value from the previous segment, at which point
		// it will be mapped into the current segment and once again present in
		// the active symbol table.
		SymbolTable _symbols;
		Segment *_segments;
		Flowgraph::NodeSet _wasCaptured;
		std::map<Flowgraph::Node*,Symbol::Type::Enum> _definitions;
		Symbol RetrieveFromPreviousSegment( Flowgraph::Node *name );
		Symbol RetrieveFromContext( Flowgraph::Node *name );
		Error::Type::Enum AssignToUndefined(
				Flowgraph::Node *name,
				Flowgraph::Node *value,
				const SourceLocation &loc );
};

// A layer is a nested scope, capable of capturing values from an outer scope
// which contains it.
class Layer : public Scope
{
	public:
		Layer( Flowgraph::Pool &pool, Scope *context );
		Scope &Context() const { return *_context; }
		Scope *Exit( const SourceLocation &loc ) { return _context; }
		void Report( const Error &err ) { _context->Report( err ); }
		bool HasReceivedReport() const
				{ return _context->HasReceivedReport(); }

	protected:
		Symbol CaptureFromContext( Flowgraph::Node *name );
		virtual Flowgraph::Node *CreateLocalReference(
				Flowgraph::Node *sym, Flowgraph::Node *val ) = 0;

	private:
		Scope *_context;
};

} // namespace Semantics

#endif // scope_h
