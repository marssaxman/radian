// Copyright 2009-2011 Mars Saxman.
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
// Radian. If not, see <http://www.gnu.org/licenses/>.


#include <queue>
#include "semantics.h"

class ModuleRef
{
	public:
		ModuleRef(
				std::string target,
				std::string directory,
				const SourceLocation &loc ):
			_target(target),
			_directory(directory),
			_sourceLoc(loc) {}
		std::string Target() const { return _target; }
		std::string Directory() const { return _directory; }
		const SourceLocation &SourceLoc() const { return _sourceLoc; }
		std::string UniqueID() const;
	private:
		std::string _target;
		std::string _directory;
		SourceLocation _sourceLoc;
};

class ModuleList : public Semantics::Importer
{
	public:
		bool Empty() const { return _modules.empty(); }
		ModuleRef Front() const { return _modules.front(); }
		void Pop() { _modules.pop(); }

	protected:
		// implementation of Semantics::Importer
		void ImportModule(
				std::string target,
				std::string directory,
				const SourceLocation &sourceLoc );

	private:
		std::queue<ModuleRef> _modules;
};

