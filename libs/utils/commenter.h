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

#include <stdbool.h>
#include <stddef.h>
#include "dll.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for storing information about comments */
typedef struct comment
{
	const char *path;	/**< Filename path */
	size_t line;		/**< Line number */
	size_t symbol;		/**< Position in line */

	const char *code;	/**< Current line in code */
} comment;


/**
 *	Create comment
 *
 *	@param	path		File path
 *	@param	line		Line number
 *
 *	@return	Comment structure
 */
EXPORTED comment cmt_create(const char *const path, const size_t line);

/**
 *	Create macro comment
 *
 *	@param	path		File path
 *	@param	line		Line number
 *	@param	symbol		Position in line
 *
 *	@return	Macro comment structure
 */
EXPORTED comment cmt_create_macro(const char *const path, const size_t line, const size_t symbol);


/**
 *	Write comment to string
 *
 *	@param	cmt			Comment
 *	@param	buffer		Destination string
 *
 *	@return	Size of сomment in buffer
 */
EXPORTED size_t cmt_to_string(const comment *const cmt, char *const buffer);


/**
 *	Find comment in code
 *
 *	@param	code		Comment
 *	@param	position	Position in code after comment
 *
 *	@return	Comment structure
 */
EXPORTED comment cmt_search(const char *const code, const size_t position);


/**
 *	Check that comment is correct
 *
 *	@param	cmt			Comment
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool cmt_is_correct(const comment *const cmt);


/**
 *	Get tag from comment
 *
 *	@param	cmt			Comment
 *	@param	buffer		Tag entry location
 *
 *	@return Size of tag
 */
EXPORTED size_t cmt_get_tag(const comment *const cmt, char *const buffer);

/**
 *	Get current code line
 *
 *	@param	cmt			Comment
 *	@param	buffer		Code line entry location
 *
 *	@return Size of code line
 */
EXPORTED size_t cmt_get_code_line(const comment *const cmt, char *const buffer);

/**
 *	Get current filename path
 *
 *	@param	cmt			Comment
 *	@param	buffer		Path entry location
 *
 *	@return Size of path
 */
EXPORTED size_t cmt_get_path(const comment *const cmt, char *const buffer);

/**
 *	Get normalized line number in code
 *
 *	@param	cmt			Comment
 *
 *	@return	Line number
 */
EXPORTED size_t cmt_get_line(const comment *const cmt);

/**
 *	Get normalized position in line
 *
 *	@param	cmt			Comment
 *
 *	@return	Position in line
 */
EXPORTED size_t cmt_get_symbol(const comment *const cmt);

#ifdef __cplusplus
} /* extern "C" */
#endif
