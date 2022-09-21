/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "syntax.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Write abstract syntax tree
 *
 *	@param	path			File path
 *	@param	sx				Syntax structure
 */
void write_tree(const char *const path, syntax *const sx);

/**
 *	Write type spelling
 *
 *	@param	sx				Syntax structure
 *	@param	type			Type
 *	@param	buffer			Buffer
 *
 *	@return	Return printf-like value
 */
int write_type_spelling(const syntax *const sx, const item_t type, char *const buffer);

/**
 *	Write virtual machine codes
 *
 *	@param	path			File path
 *	@param	memory			Instructions table
 */
void write_codes(const char *const path, const vector *const memory);

#ifdef __cplusplus
} /* extern "C" */
#endif
