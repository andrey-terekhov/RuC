/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
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
#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Output new tree
 *
 *	@param	tree	Tree table
 *	@param	path	File path
 */
void tree_print(vector *const tree, const char *const path);


/**
 *	Output tables and tree
 *
 *	@param	sx		Syntax structure
 *	@param	path	File path
 */
void tables_and_tree(const syntax *const sx, const char *const path);

/**
 *	Output tables and codes
 *
 *	@param	sx		Syntax structure
 *	@param	path	File path
 */
void tables_and_codes(const syntax *const sx, const char *const path);

#ifdef __cplusplus
} /* extern "C" */
#endif
