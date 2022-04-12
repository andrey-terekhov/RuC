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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dll.h"

#ifdef __APPLE__
	typedef uint32_t char32_t;
#else
	#include <uchar.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Number of UTF-8 character bytes
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	Number of bytes
 */
EXPORTED size_t utf8_size(const char32_t symbol);

/**
 *	Number of UTF-8 character octets
 *
 *	@param	symbol	Character
 *
 *	@return	Number of octets
 */
EXPORTED size_t utf8_symbol_size(const char symbol);

/**
 *	Get first UTF-8 character octet index
 *
 *	@param	str		String with character
 *	@param	index	Index of character
 *
 *	@return	First octet index
 */
EXPORTED size_t utf8_to_first_byte(const char *const str, const size_t index);

/**
 *	Convert character from string to UTF-8 сharacter
 *
 *	@param	symbol	Character
 *
 *	@return	UTF-8 сharacter
 */
EXPORTED char32_t utf8_convert(const char *const symbol);

/**
 *	Write UTF-8 сharacter to string
 *
 *	@param	buffer	Output string
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	Size of сharacter in string
 */
EXPORTED size_t utf8_to_string(char *const buffer, const char32_t symbol);

/**
 *	Convert CP866 string to UTF-8 string
 *
 *	@param	src		CP866 string
 *	@param	dest	UTF-8 string
 *
 *	@return	Size of destination string
 */
EXPORTED size_t utf8_from_cp866(const char *const src, char *const dest);

/**
 *	Convert Windows-1251 string to UTF-8 string
 *
 *	@param	src		Windows-1251 string
 *	@param	dest	UTF-8 string
 *
 *	@return	Size of destination string
 */
EXPORTED size_t utf8_from_cp1251(const char *const src, char *const dest);

/**
 *	Convert UTF-8 string to CP866 string
 *
 *	@param	src		UTF-8 string
 *	@param	dest	CP866 string
 *
 *	@return	Size of destination string
 */
EXPORTED size_t utf8_to_cp866(const char *const src, char *const dest);

/**
 *	Convert UTF-8 string to Windows-1251 string
 *
 *	@param	src		UTF-8 string
 *	@param	dest	Windows-1251 string
 *
 *	@return	Size of destination string
 */
EXPORTED size_t utf8_to_cp1251(const char *const src, char *const dest);

/**
 *	Transliteration from russian to english
 *
 *	@param	src		Russian string
 *	@param	dest	English string
 *
 *	@return	Size of destination string
 */
EXPORTED size_t utf8_transliteration(const char *const src, char *const dest);

/**
 *	Convert UTF-8 symbol to upper case
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	Upper case character
 */
EXPORTED char32_t utf8_to_upper(const char32_t symbol);

/**
 *	Convert hexadecimal digit to number
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	Corresponding number
 */
EXPORTED uint8_t utf8_to_number(const char32_t symbol);

/**
 *	Check if сharacter is russian letter
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool utf8_is_russian(const char32_t symbol);

/**
 *	Check if сharacter is english or russian letter
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool utf8_is_letter(const char32_t symbol);

/**
 *	Check if сharacter is decimal digit
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool utf8_is_digit(const char32_t symbol);

/**
 *	Check if сharacter is hexadecimal digit
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool utf8_is_hexa_digit(const char32_t symbol);

/**
 *	Check if сharacter is 'E', 'e', 'Е' or 'е'
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool utf8_is_power(const char32_t symbol);

#ifdef __cplusplus
} /* extern "C" */
#endif
