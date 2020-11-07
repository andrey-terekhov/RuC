/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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
#include <uchar.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Number of UTF-8 character octets
 *
 *	@param	symbol	Character
 *
 *	@return	Number of octets
 */
size_t symbol_size(const char symbol);

/**
 *	Convert character of string to UTF-8 сharacter
 *
 *	@param	symbol	Character
 *
 *	@return	UTF-8 сharacter
 */
char32_t to_utf_8(const char *const symbol);

/**
 *	Write UTF-8 сharacter to string
 *
 *	@param	buffer	Output string
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	Size of сharacter in string
 */
size_t to_string(char *const buffer, const char32_t symbol);

/**
 *	Checks if сharacter is russian
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int is_russian(const char32_t symbol);

#ifdef __cplusplus
} /* extern "C" */
#endif
