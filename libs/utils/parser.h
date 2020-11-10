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

#include <limits.h>
#include <stddef.h>
#include "utils_internal.h"

 
#define MAX_PATHS 1024
#define MAX_FLAGS 128
#define MAX_STRING 256


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for parsing start arguments of program */
typedef struct workspace
{
	char files[MAX_PATHS][MAX_STRING];	/** Files list */
	size_t files_num;					/** Number of files */

	char dirs[MAX_PATHS][MAX_STRING];	/** Directories list */
	size_t dirs_num;					/** Number of directories */

	char flags[MAX_FLAGS][MAX_STRING];	/** Flags list */
	size_t flags_num;					/** Number of flags */

	char output[MAX_STRING];			/** Output file name */
	int was_error;						/** @c 0 if no errors */
} workspace;


/**
 *	Parse command line arguments
 *
 *	@param	argc		Number of command line arguments
 *	@param	argv		Command line arguments
 *
 *	@return	Workspace structure
 */
UTILS_EXPORTED workspace ws_parse_args(const int argc, const char *const *const argv);


/**
 *	Create empty workspace
 *
 *	@return	Workspace structure
 */
UTILS_EXPORTED workspace ws_create();


/**
 *	Add file path to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	path		File path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_file(workspace *const ws, const char *const path);

/**
 *	Add files paths to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	paths		Files paths
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_files(workspace *const ws, const char *const *const paths);


/**
 *	Add include directory to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	path		Directory path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_dir(workspace *const ws, const char *const path);

/**
 *	Add include directories to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	path		Directories paths
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_dirs(workspace *const ws, const char *const *const paths);


/**
 *	Add flag to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	flag		Flag
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_flag(workspace *const ws, const char *const flag);

/**
 *	Add flags to workspace
 *
 *	@param	ws			Workspace structure
 *	@param	flag		Flags
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_add_flags(workspace *const ws, const char *const *const flags);


/**
 *	Set output file name
 *
 *	@param	ws			Workspace structure
 *	@param	path		Output file name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_set_output(workspace *const ws, const char *const path);


/**
 *	Check that workspace structure is correct
 *
 *	@param	ws			Workspace structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int ws_is_correct(const workspace *const ws);


/**
 *	Get files list from workspase
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Files list
 */
UTILS_EXPORTED const char *const *ws_get_file_list(const workspace *const ws);

/**
 *	Get directories list from workspase
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Directories list
 */
UTILS_EXPORTED const char *const *ws_get_dir_list(const workspace *const ws);

/**
 *	Get flags list from workspase
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Flags list
 */
UTILS_EXPORTED const char *const *ws_get_flag_list(const workspace *const ws);


/**
 *	Get output file name
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Output file name
 */
UTILS_EXPORTED const char *ws_get_output(const workspace *const ws);


/**
 *	Clear workspase structure
 *
 *	@param	ws			Workspace structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int ws_clear(workspace *const ws);

#ifdef __cplusplus
} /* extern "C" */
#endif
