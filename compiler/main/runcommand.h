// Copyright 2013-2016 Mars Saxman.
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

#ifndef main_runcommand_h
#define main_runcommand_h

#include <deque>
#include <string>
#include <set>
#include "main/error.h"
#include "main/modulelist.h"
#include "main/command.h"

class RunCommand : public Command
{
	public:
		RunCommand( std::string &execpath );
		std::string Description() const
		{
			return "Compile a program and run it";
		}
		std::string Help() const;
		int Run( std::deque<std::string> args );
	private:
        std::string _programRootDir;
		std::string _executablePath;
        std::string _targetTriple;

        void LocateProgramRootDir( std::string mainFile );
		bool CompileProgram( std::string filepath, ModuleList &modules );
		bool CompileModules( ModuleList &modules );
		bool CompileModule( const ModuleRef &item, ModuleList &worklist );
        int LoadFileForTarget(
                const std::string &dirPath,
                const std::string &name,
                std::string *filePath,
                std::string *output);
};


#endif // runcommand_h
