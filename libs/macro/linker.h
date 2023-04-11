/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Egor Anikin
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


#define TAG_LINKER "linker"


#ifdef __cplusplus
extern "C" {
#endif

/** Linker structure */
typedef struct linker
{
	workspace *ws;				/**< Sources files */
	const size_t sources; 		/**< Number of sources files */

	vector included;			/**< List of included files */

	size_t current; 			/**< Index of current file */
} linker;


/**
 *	Create linker structure
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Linker structure
 */
linker linker_create(workspace *const ws);


/**
 *	Link source file forcibly
 *
 *	@param	lk			Linker structure
 *	@param	index		Index of file
 *
 *	@return	File input stream
 */
universal_io linker_add_source(linker *const lk, const size_t index);

/**
 *	Link header file safely
 *
 *	@param	lk			Linker structure
 *	@param	index		Index of file
 *
 *	@return	File input stream
 */
universal_io linker_add_header(linker *const lk, const size_t index);


/**
 *	Search file into same folder first
 *
 *	@param	lk			Linker structure
 *	@param	file		File name
 *
 *	@return	Index of file, @c SIZE_MAX on failure
 */
size_t linker_search_internal(linker *const lk, const char *const file);

/**
 *	Search file into specified paths first
 *
 *	@param	lk			Linker structure
 *	@param	file		File name
 *
 *	@return	Index of file, @c SIZE_MAX on failure
 */
size_t linker_search_external(linker *const lk, const char *const file);


/**
 *	Get current file path
 *
 *	@param	lk			Linker structure
 *
 *	@return	Path to file, @c NULL on failure
 */
const char* linker_current_path(const linker *const lk);

/**
 *	Get current path index
 *
 *	@param	lk			Linker structure
 *
 *	@return	Index of file, @c SIZE_MAX on failure
 */
size_t linker_get_index(const linker *const lk);

/**
 *	Set current path index
 *
 *	@param	lk			Linker structure
 *	@param	index		Index of file
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int linker_set_index(linker *const lk, const size_t index);


/**
 *	Get number of linker files
 *
 *	@param	lk			Linker structure
 *
 *	@return	Number of linker files, @c SIZE_MAX on failure
 */
size_t linker_size(const linker *const lk);

/**
 *	Check that linker is correct
 *
 *	@param	lk			Linker structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool linker_is_correct(const linker *const lk);


/**
 *	Free allocated memory
 *
 *	@param	lk			Linker structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int linker_clear(linker *const lk);

#ifdef __cplusplus
} /* extern "C" */
#endif
