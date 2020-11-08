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

#include <stddef.h>
#include "utils_internal.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for storing information about comments */
typedef struct comment
{
	char path[100/*MAX_STRING*/];
	size_t line;
	size_t symbol;
	int type;
} comment;

/**
 *	Create comment
 *
 *	@param	path	File path
 *	@param	line	Line nomber
 *
 *	@return	Result
 */
UTILS_EXPORTED comment cmt_create(const char *const path, const size_t line);

/**
 *	Create macro comment
 *
 *	@param	path	File path
 *	@param	line	Line nomber
  *	@param	symbol	Position in line
 *
 *	@return	Result
 */
UTILS_EXPORTED comment cmt_create_macro(const char *const path, const size_t line, const size_t symbol);


/**
 *	Add comment to buffer
 *
 *	@param	cmt		Comment
 *	@param	buffer	Destination line
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
UTILS_EXPORTED int cmt_to_buffer(const comment *const cmt, char *const buffer);

/**
 *	Find comment in code
 *
 *	@param	code	Comment
 *	@param	position	Position in code after comment
 *
 *	@return	Result
 */
UTILS_EXPORTED comment cmt_search(const char *const code, const size_t position);


/**
 *	Check that comment is correct
 *
 *	@param	cmt	Comment
 *
 *	@return	@c 1 if correct, @c 0 if incorrect
 */
UTILS_EXPORTED int cmt_is_correct(const comment *const cmt);

/**
 *	Check that comment is macro
 *
 *	@param	cmt	Comment
 *
 *	@return	@c 1 if macro, @c 0 if not macro
 */
UTILS_EXPORTED int cmt_is_macro(const comment *const cmt);


/**
 *	Extract path from comment
 *
 *	@param	cmt	Comment
 *
 *	@return	result
 */
UTILS_EXPORTED const char * cmt_get_path(const comment *const cmt);

/**
 *	Extract line from comment
 *
 *	@param	cmt	Comment
 *
 *	@return	result
 */
UTILS_EXPORTED size_t cmt_get_line(const comment *const cmt);

/**
 *	Extract symbol from comment
 *
 *	@param	cmt	Comment
 *
 *	@return	result
 */
UTILS_EXPORTED size_t cmt_get_symbol(const comment *const cmt);

#ifdef __cplusplus
} /* extern "C" */
#endif
