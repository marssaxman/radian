// Copyright 2010-2012 Mars Saxman.
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

#include "llvmutils.h"
#include <llvm/ADT/Triple.h>
#include <llvm/ADT/ArrayRef.h>
#include <vector>
#include <stdarg.h>

/**
 * Determines the appropriate data layout for the given target triple.
 * @param triple The target triple.
 * @return The correct data layout or an empty string if this target is unknown.
 *
 * @internal The values in this function were taken directly from Clang's
 *           'Targets.cpp'.
 */
std::string DataLayoutForTriple(std::string triple)
{
	llvm::Triple parsedTriple( llvm::Triple::normalize( triple ) );
	llvm::Triple::OSType os = parsedTriple.getOS();
	
	switch (parsedTriple.getArch()) {
		case llvm::Triple::x86:
			if (os == llvm::Triple::Darwin) {
				return ("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
						"i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-"
						"a0:0:64-f80:128:128-n8:16:32");
			} else if (os == llvm::Triple::Win32) {
				return ("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
						"i64:64:64-f32:32:32-f64:64:64-f80:128:128-v64:64:64-"
						"v128:128:128-a0:0:64-f80:32:32-n8:16:32");
			} else if (os == llvm::Triple::Cygwin) {
				return ("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
						"i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-"
						"a0:0:64-f80:32:32-n8:16:32");
			} else {
				return ("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
						"i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-"
						"a0:0:64-f80:32:32-n8:16:32");
			}
		case llvm::Triple::x86_64:
			return ("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
					"i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-"
					"a0:0:64-s0:64:64-f80:128:128-n8:16:32:64");
		case llvm::Triple::ppc:
			return ("E-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
					"i64:64:64-f32:32:32-f64:64:64-v128:128:128-n32");
		case llvm::Triple::ppc64:
			return ("E-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
					"i64:64:64-f32:32:32-f64:64:64-v128:128:128-n32:64");
		case llvm::Triple::arm:
			if (parsedTriple.getArchName().startswith( "thumb" )) {
				return ("e-p:32:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32-"
						"i64:64:64-f32:32:32-f64:64:64-"
						"v64:64:64-v128:128:128-a0:0:32-n32");
			} else {
				return ("e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-"
						"i64:64:64-f32:32:32-f64:64:64-"
						"v64:64:64-v128:128:128-a0:0:64-n32");
			}
		default:
			// We don't know anything about this architecture. Sadface.
			return "";
	}
}

/**
 * Creates a FunctionType given a return type and a bunch of argument types.
 * @param retTy The return type. Cannot be NULL.
 * @param ... The argument types, as a NULL-terminated list.
 * @return The created FunctionType.
 */
llvm::FunctionType * GenerateFunctionType(llvm::Type *retTy, ...) {
	assert(retTy != NULL);
	
    va_list vl;
    va_start(vl, retTy);
    
    std::vector<llvm::Type *> argTypes;
    while (1) {
        llvm::Type *argType = va_arg(vl, llvm::Type *);
        if (argType == NULL) break;
        
        argTypes.push_back(argType);
    }
    
    return llvm::FunctionType::get(retTy, argTypes, false);
}
