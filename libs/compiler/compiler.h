/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#pragma once

#include "dll.h"
#include "workspace.h"


//#define GENERATE_MACRO


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Compile code from workspace
 *
 *	@param	ws		Compiler workspace
 *
 *	@return	Status code
 */
EXPORTED int compile(workspace *const ws);

/**
 *	Compile RuC virtual machine code from workspace
 *
 *	@param	ws		Compiler workspace
 *
 *	@return	Status code
 */
EXPORTED int compile_to_vm(workspace *const ws);

/**
 *	Compile LLVM code from workspace
 *
 *	@param	ws		Compiler workspace
 *
 *	@return	Status code
 */
EXPORTED int compile_to_llvm(workspace *const ws);


/**
 *	Compile code from terminal arguments
 *
 *	@param	argc	Number of command line arguments
 *	@param	argv	Command line arguments
 *
 *	@return	Status code
 */
EXPORTED int auto_compile(const int argc, const char *const *const argv);

/**
 *	Compile RuC virtual machine code from terminal arguments
 *
 *	@param	argc	Number of command line arguments
 *	@param	argv	Command line arguments
 *
 *	@return	Status code
 */
EXPORTED int auto_compile_to_vm(const int argc, const char *const *const argv);

/**
 *	Compile LLVM code from terminal arguments
 *
 *	@param	argc	Number of command line arguments
 *	@param	argv	Command line arguments
 *
 *	@return	Status code
 */
EXPORTED int auto_compile_to_llvm(const int argc, const char *const *const argv);


/**
 *	Compile RuC virtual machine code with no macro
 *
 *	@param	path	File path
 *
 *	@return	Status code
 */
EXPORTED int no_macro_compile_to_vm(const char *const path);

/**
 *	Compile LLVM code with no macro
 *
 *	@param	path	File path
 *
 *	@return	Status code
 */
EXPORTED int no_macro_compile_to_llvm(const char *const path);

#ifdef __cplusplus
} /* extern "C" */
#endif
