/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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
#include "strings.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define MAX_PATHS    128
#define MAX_ARG_SIZE 1024


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for parsing start arguments of program */
typedef struct workspace
{
    strings files; /**< Files list */
    strings dirs;  /**< Directories list */
    strings flags; /**< Flags list */

    char output[MAX_ARG_SIZE]; /**< Output file name */
    bool was_error;            /**< @c 0 if no errors */
} workspace;


/**
 *	Parse command line arguments
 *
 *	@param	argc		Number of command line arguments
 *	@param	argv		Command line arguments
 *
 *	@return	Workspace structure
 */
EXPORTED workspace ws_parse_args(const int argc, const char *const *const argv);


/**
 *	Create empty workspace
 *
 *	@return	Workspace structure
 */
EXPORTED workspace ws_create(void);


/**
 *	Add file path to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	path		File path
 *
 *	@return	File index, @c SIZE_MAX on failure
 */
EXPORTED size_t ws_add_file(workspace *const ws, const char *const path);

/**
 *	Add files paths to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	paths		Files paths
 *	@param	num			Number of files
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int ws_add_files(workspace *const ws, const char *const *const paths, const size_t num);


/**
 *	Add include directory to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	path		Directory path
 *
 *	@return	Directory index, @c SIZE_MAX on failure
 */
EXPORTED size_t ws_add_dir(workspace *const ws, const char *const path);

/**
 *	Add include directories to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	paths		Directories paths
 *	@param	num			Number of directories
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int ws_add_dirs(workspace *const ws, const char *const *const paths, const size_t num);


/**
 *	Add flag to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	flag		Flag
 *
 *	@return	Flag index, @c SIZE_MAX on failure
 */
EXPORTED size_t ws_add_flag(workspace *const ws, const char *const flag);

/**
 *	Add flags to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	flags		Flags
 *	@param	num			Number of flags
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int ws_add_flags(workspace *const ws, const char *const *const flags, const size_t num);


/**
 *	Set output file name
 *
 *	@param	ws			Workspace structure
 *	@param	path		Output file name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int ws_set_output(workspace *const ws, const char *const path);


/**
 *	Check that workspace structure is correct
 *
 *	@param	ws			Workspace structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool ws_is_correct(const workspace *const ws);

/**
 *	Check that workspace contains flag
 *
 *	@param	ws			Workspace structure
 *	@param	flag		Flag to check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool ws_has_flag(const workspace *const ws, const char *const flag);


/**
 *	Get file by index from workspase
 *
 *	@param	ws			Workspace structure
 *	@param	index		File index in list
 *
 *	@return	File
 */
EXPORTED const char *ws_get_file(const workspace *const ws, const size_t index);

/**
 *	Get number of files
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Number of files
 */
EXPORTED size_t ws_get_files_num(const workspace *const ws);

/**
 *	Get directory by index from workspase
 *
 *	@param	ws			Workspace structure
 *	@param	index		Directory index in list
 *
 *	@return	Directory
 */
EXPORTED const char *ws_get_dir(const workspace *const ws, const size_t index);

/**
 *	Get number of directories
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Number of directories
 */
EXPORTED size_t ws_get_dirs_num(const workspace *const ws);

/**
 *	Get flag by index from workspase
 *
 *	@param	ws			Workspace structure
 *	@param	index		Flag index in list
 *
 *	@return	Flag
 */
EXPORTED const char *ws_get_flag(const workspace *const ws, const size_t index);

/**
 *	Get number of flags
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Number of flags
 */
EXPORTED size_t ws_get_flags_num(const workspace *const ws);


/**
 *	Get output file name
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Output file name, @c NULL on default
 */
EXPORTED const char *ws_get_output(const workspace *const ws);


/**
 *	Free allocated memory
 *
 *	@param	ws			Workspace structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int ws_clear(workspace *const ws);

#ifdef __cplusplus
} /* extern "C" */
#endif
