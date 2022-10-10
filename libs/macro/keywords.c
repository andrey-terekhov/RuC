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

#include "keywords.h"


extern inline bool kw_is_correct(const keyword_t kw);


static int add_keyword(map *const as
	, const char32_t *const eng, const char32_t *const eng_up
	, const char32_t *const rus, const char32_t *const rus_up
	, const keyword_t kw)
{
	return map_add_by_utf8(as, eng, kw) == SIZE_MAX || map_add_by_utf8(as, eng_up, kw) == SIZE_MAX
		|| map_add_by_utf8(as, rus, kw) == SIZE_MAX || map_add_by_utf8(as, rus_up, kw) == SIZE_MAX;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int kw_add(map *const as)
{
	if (!map_is_correct(as))
	{
		return -1;
	}

	return add_keyword(as, U"#include",		U"#INCLUDE",	U"#подключить",		U"#ПОДКЛЮЧИТЬ",		KW_INCLUDE)
		|| add_keyword(as, U"#line",		U"#LINE",		U"#строка",			U"#СТРОКА",			KW_LINE)

		|| add_keyword(as, U"#define",		U"#DEFINE",		U"#определить",		U"#ОПРЕДЕЛИТЬ",		KW_DEFINE)
		|| add_keyword(as, U"#set",			U"#SET",		U"#переопределить",	U"#ПЕРЕОПРЕДЕЛИТЬ",	KW_SET)
		|| add_keyword(as, U"#undef",		U"#UNDEF",		U"#разопределить",	U"#РАЗОПРЕДЕЛИТЬ",	KW_UNDEF)

		|| add_keyword(as, U"#macro",		U"#MACRO",		U"#макро",			U"#МАКРО",			KW_MACRO)
		|| add_keyword(as, U"#endm",		U"#ENDM",		U"#конецм",			U"#КОНЕЦМ",			KW_ENDM)

		|| add_keyword(as, U"#ifdef",		U"#IFDEF",		U"#еслибыл",		U"#ЕСЛИБЫЛ",		KW_IFDEF)
		|| add_keyword(as, U"#ifndef",		U"#IFNDEF",		U"#еслинебыл",		U"#ЕСЛИНЕБЫЛ",		KW_IFNDEF)
		|| add_keyword(as, U"#if",			U"#IF",			U"#если",			U"#ЕСЛИ",			KW_IF)
		|| add_keyword(as, U"#elif",		U"#ELIF",		U"#инесли",			U"#ИНЕСЛИ",			KW_ELIF)
		|| add_keyword(as, U"#else",		U"#ELSE",		U"#иначе",			U"#ИНАЧЕ",			KW_ELSE)
		|| add_keyword(as, U"#endif",		U"#ENDIF",		U"#конецесли",		U"#КОНЕЦЕСЛИ",		KW_ENDIF)

		|| add_keyword(as, U"#eval",		U"#EVAL",		U"#вычислить",		U"#ВЫЧИСЛИТЬ",		KW_EVAL)

		|| add_keyword(as, U"#while",		U"#WHILE",		U"#пока",			U"#ПОКА",			KW_WHILE)
		|| add_keyword(as, U"#endw",		U"#ENDW",		U"#конецп",			U"#КОНЕЦП",			KW_ENDW);
}
