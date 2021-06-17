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
 *	Output tables and tree
 *
 *	@param	path			File path
 *	@param	identifiers		Identifiers table
 *	@param	modes			Modes table
 *	@param	tree			Tree table
 */
void tables_and_tree(const char *const path
	, const vector *const identifiers
	, const vector *const modes
	, vector *const tree);

/**
 *	Output tables and codes
 *
 *	@param	path			File path
 *	@param	functions		Functions table
 *	@param	processes		Init processes table
 *	@param	memory			Memory table
 */
void tables_and_codes(const char *const path
	, const vector *const functions
	, const vector *const processes
	, const vector *const memory);

#ifdef __cplusplus
} /* extern "C" */
#endif
