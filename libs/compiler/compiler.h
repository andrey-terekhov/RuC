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


#ifdef _MSC_VER
	#define COMPILER_EXPORTED __declspec(dllexport)
#else
	#define COMPILER_EXPORTED
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Structure to carry information about a single compiled file */
typedef struct compiler_workspace_file
{
	struct compiler_workspace_file *next; /** Next file */
	char *path;							  /** Path to the file associated with the current object */
} compiler_workspace_file;

/** Compiler workspace error codes */
typedef enum compiler_workspace_errno
{
	COMPILER_WS_EOK = 0,   /** No error occured */
	COMPILER_WS_ENOOUTPUT, /** No output file set */
	COMPILER_WS_EFILEADD,  /** File adding problem */
	COMPILER_WS_ENOINPUT,  /** No input files provided */
} compiler_workspace_errno;

/** Structure to carry information about workspace propagation errors */
typedef struct compiler_workspace_error
{
	compiler_workspace_errno code; /** Error code */
} compiler_workspace_error;

/** Structure to carry information about the whole build workspace */
typedef struct compiler_workspace
{
	compiler_workspace_file *files; /** A single-linked list of files */
	compiler_workspace_error error; /** Workspace propagation error */
	char *output_file;				/** Output file name */
	int number_of_files;			/** Number of files */
} compiler_workspace;


/**
 *	Compile a single file to a default output executable.
 *
 *	@param	path	Path to a single file
 *
 *	@return	Status code
 *
 *	@remark	This function wraps all workspace creation, propagation, compilation,
 *			and cleanup routines and gives a single result.
 */
COMPILER_EXPORTED int compiler_compile(const char *path);

/**
 *	Create a workspace for compilation
 *
 *	@return	Newly allocated compiler_workspace, or @c NULL
 */
COMPILER_EXPORTED compiler_workspace *compiler_workspace_create();

/**
 *	Create a workspace for compilation
 *
 *	@param	workspace	compiler_workspace to free
 *
 *	@remark	The function does nothing with @c NULL workspace
 */
COMPILER_EXPORTED void compiler_workspace_free(compiler_workspace *workspace);

/**
 *	Add a file to a workspace
 *
 *	@param	workspace	compiler_workspace to add file to
 *	@param	path		Path to file to add to workspace
 *
 *	@return	Newly allocated workspace file (already added to workspace),
 *			or @c NULL in case of failure
 */
COMPILER_EXPORTED compiler_workspace_file *compiler_workspace_add_file(compiler_workspace *workspace, const char *path);

/**
 *	Get a workspace out of frontend parameters
 *
 *	@param	argc	Number of arguments
 *	@param	argv	String arguments to compiler, starting with the name of
 *					compiler executable
 *
 *	@return	Newly allocated compiler_workspace, or @c NULL
 */
COMPILER_EXPORTED compiler_workspace *compiler_get_workspace(int argc, const char *argv[]);

/**
 *	Compile RuC files set as compiler arguments
 *
 *	@param	argc	Number of arguments
 *	@param	argv	String arguments to compiler, starting with the name of
 *					compiler executable
 *
 *	@return	Status code
 */
COMPILER_EXPORTED int compiler_workspace_compile(compiler_workspace *workspace);

/**
 *	Retrieve a textual representation of workspace propagation error
 *
 *	@param	error	Pointer to compiler_workspace_error structure
 *
 *	@return	Friendly textual representation of error, to be freed with free()
 */
COMPILER_EXPORTED char *compiler_workspace_error2str(compiler_workspace_error *error);

#ifdef __cplusplus
} /* extern "C" */
#endif
