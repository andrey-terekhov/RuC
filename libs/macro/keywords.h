/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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
#include "map.h"


#define MAX_KEYWORD_SIZE 64


#ifdef __cplusplus
extern "C" {
#endif

typedef enum KEYWORD
{
	ERROR_KEYWORD,
	BEGIN_KEYWORD,

	KW_INCLUDE,					/**< '#include'	keyword	*/
	KW_LINE,					/**< '#line'	keyword	*/

	KW_DEFINE,					/**< '#define'	keyword	*/
	KW_SET,						/**< '#set'		keyword	*/
	KW_UNDEF,					/**< '#undef'	keyword	*/

	KW_MACRO,					/**< '#macro'	keyword	*/
	KW_ENDM,					/**< '#endm'	keyword	*/

	KW_IFDEF,					/**< '#ifdef'	keyword	*/
	KW_IFNDEF,					/**< '#ifndef'	keyword	*/
	KW_IF,						/**< '#if'		keyword	*/
	KW_ELIF,					/**< '#elif'	keyword	*/
	KW_ELSE,					/**< '#else'	keyword	*/
	KW_ENDIF,					/**< '#endif'	keyword	*/

	KW_EVAL,					/**< '#eval'	keyword	*/

	KW_WHILE,					/**< '#while'	keyword	*/
	KW_ENDW,					/**< '#endw'	keyword	*/

	END_KEYWORD,
	NON_KEYWORD,
} keyword_t;


/**
 *	Initialize map structure by adding keywords
 *
 *	@param	as				Map structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int kw_add(map *const as);

/**
 *	Get corresponding begin block keyword.
 *
 *	@param	directive		Directive spelling
 *	@param	buffer			Output string
 *
 *	@return	Size of written characters
 */
size_t kw_without(const char *const directive, char *const buffer);

/**
 *	Get corresponding @c #else keyword spelling.
 *
 *	@param	directive		Directive spelling
 *	@param	buffer			Output string
 *
 *	@return	Size of written characters
 */
size_t kw_after(const char *const directive, char *const buffer);

/**
 *	Check that keyword is correct
 *
 *	@param	kw				Keyword
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool kw_is_correct(const keyword_t kw)
{
	return BEGIN_KEYWORD < kw && kw < END_KEYWORD;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
