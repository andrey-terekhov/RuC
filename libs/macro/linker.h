/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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
#include "vector.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for connecting files */
typedef struct linker
{
	workspace *ws;				/**< Initial arguments */
	size_t sources; 			/**< Number of sources files */

	vector included;			/**< List of already added files */

	size_t current; 			/**< Index of the current file */
} linker;


/**
 *	Create linker structure
 *
 *	@param	ws		Workspace structure	 
 *
 *	@return	Linker structure
 */
linker linker_create(workspace *const ws);


/**
 *	Open new source file
 *
 *	@param	lk		Linker structure
 *	@param	index	File index in workspace
 *
 *	@return	File
 */
universal_io linker_add_source(linker *const lk, const size_t index);

/**
 *	Open new incude file
 *
 *	@param	lk		Linker structure
 *	@param	index	File index in workspace
 *
 *	@return	File
 */
universal_io linker_add_header(linker *const lk, const size_t index);


/**
 *	Find a file in the same folder
 *
 *	@param	lk		Linker structure
 *	@param	file	File name
 *
 *	@return	File index in workspace, @c SIZE_MAX on failure
 */
size_t linker_search_internal(linker *const lk, const char *const file);

/**
 *	Find file through dirs in workspace
 *
 *	@param	lk		Linker structure
 *	@param	file	File name
 *
 *	@return	File index in workspace, @c SIZE_MAX on failure
 */
size_t linker_search_external(linker *const lk, const char *const file);


/**
 *	Get current file name from linker
 *
 *	@param	lk		Linker structure
 *
 *	@return	File name, @c NULL on failure
 */
const char* linker_current_path(const linker *const lk);


/**
 *	Get linker size
 *
 *	@param	lk		Linker structure
 *
 *	@return	Size of linker, @c SIZE_MAX on failure
 */
size_t linker_size(const linker *const lk);

/**
 *	Check that linker structure is correct
 *
 *	@param	lk		Linker structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool linker_is_correct(const linker *const lk);


/**
 *	Clear linker structure
 *
 *	@param	lk		Linker structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int linker_clear(linker *const lk);

#ifdef __cplusplus
} /* extern "C" */
#endif
