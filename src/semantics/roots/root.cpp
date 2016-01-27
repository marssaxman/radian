// Copyright 2012-2016 Mars Saxman.
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

#include "semantics/roots/root.h"

using namespace Semantics;
using namespace Flowgraph;

Root::Root( Flowgraph::Pool &pool, Reporter &log ):
	Scope(pool),
	_errors(log)
{
}

Root::~Root()
{
	_pool.Validate( _errors.HasReceivedReport() );
}

void Root::Report( const Error &err )
{
	_errors.Report( err );
}

Scope *Root::Exit( const SourceLocation &loc )
{
	ExitRoot();
	// There is no outer scope past this one.
	return NULL;
}

Symbol Root::CaptureFromContext( Node *name )
{
	// There is nothing above the root scope.
	return Symbol::Undefined();
}

void Root::RebindInContext(
		Node *sym, Node *value, const SourceLocation &loc )
{
	// The source file is the root of the namespace, so there is nothing to
	// redefine outside it.
	ReportError( Error::Type::Undefined, loc );
}
