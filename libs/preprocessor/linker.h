/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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

#include "workspace.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct environment environment;

/**	Structure for connecting files */
typedef struct linker
{
	workspace *ws;				/**< Initial arguments */

	int included[MAX_PATHS];	/**< List of already added files */
	size_t count; 				/**< Number of added files */

	size_t current; 			/**< Index of the current file */

	environment *env;			/**< Preprocessor environment */
} linker;


/**
 *	Create linker structure
 *
 *	@param	ws		Workspace
 *	@param	env		Preprocessor environment
 *
 *	@return	Linker structure
 */
linker lk_create(workspace *const ws, environment *const env);

/**
 *	Include current file from environment to target output
 *
 *	@param	env		Preprocessor environment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
size_t lk_include(linker *const lk);

/**
 *	Get file name from linker
 *
 *	@param	lk		Preprocessor linker
 *	@param	num		File index
 *
 *	@return	File
 */
const char *lk_get_path(const linker *const lk, size_t num);

/**
 *	Get number of files from linker
 *
 *	@param	lk		Preprocessor linker
 *
 *	@return	Number of files
 */
size_t lk_get_count(const linker *const lk);

/**
 *	Checks if a file is included
 *
 *	@param	lk		Preprocessor linker
 *	@param	index	File index
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int lk_is_included(const linker *const lk, size_t index);

/**
 *	Open file from linker
 *
 *	@param	lk		Preprocessor linker
 *	@param	index	File index
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int lk_open_file(linker *const lk, const size_t index);

/**
 *	Get current file index from linker
 *
 *	@param	lk		Preprocessor linker
 *
 *	@return	Index file
 */
size_t lk_get_current(const linker *const lk);

/**
 *	Set current file from linker
 *
 *	@param	lk		Preprocessor linker
 *	@param	index	File index
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int lk_set_current(linker *const lk, size_t index);

#ifdef __cplusplus
} /* extern "C" */
#endif
