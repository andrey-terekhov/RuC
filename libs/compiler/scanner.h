/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev, Ilya Andreev
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

#include "analyzer.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Read next character from io
 *
 *	@param	context	Analyzer structure
 *
 *	@return	character
 */
char32_t get_char(analyzer *const context);

/**
 *	Lex next token from io
 *
 *	@param	context	Analyzer structure
 *
 *	@return	token
 */
int lex(analyzer *const context);

#ifdef __cplusplus
} /* extern "C" */
#endif
