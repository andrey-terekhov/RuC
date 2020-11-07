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

/**
 *	Add comment in line
 *
 *	@param	comment	Text of comment
 *	@param	path	Part of code
 *	@param	line	Nomber of line
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int comment(char *const comment, const char *const path, const size_t line);

/**
 *	Add comment before symbol in line
 *
 *	@param	comment	Text of comment
 *	@param	path	Part of code
 *	@param	line	Nomber of line
 *	@param	symbol	Position
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int macro_comment(char *const comment, const char *const path, const size_t line, const size_t symbol);

/**
 *	Search comment in code
 *
 *	@param	code		Line of code
 *	@param	position	Position in line after comment
 *
 *	@return	@c string Text of comment
 */
UTILS_EXPORTED const char * search_comment(const char *const code, const size_t position);

/**
 *	
 *
 *	@param	comment	Text of comment
 *	@param	path	Part of code
 *	@param	line	Nomber of line
 *	@param	symbol	Position in line after comment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int extract_comment(const char *const comment, char *const path, size_t *const line, size_t *const symbol);

#ifdef __cplusplus
} /* extern "C" */
#endif
