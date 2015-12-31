// Copyright 2013 Mars Saxman.
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

#ifndef runcommand_h
#define runcommand_h

#include <deque>
#include <string>
#include <set>
#include <llvm/IR/LLVMContext.h>
#include "error.h"
#include "modulelist.h"
#include "platform.h"
#include "command.h"

class RunCommand : public Command
{
	public:
		RunCommand( Platform &os, std::string &execpath );
		std::string Description() const
		{
			return "Compile a program and run it";
		}
		std::string Help() const;
		int Run( std::deque<std::string> args );
	private:
		Platform &_os;
        std::string _programRootDir;
		std::string _executablePath;
        std::string _targetTriple;

		// Collected compilation data
		llvm::LLVMContext _backend;
		llvm::Module *_compiledProgram;
		std::vector<llvm::Module *> _compiledModules;

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