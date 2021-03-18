/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
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

#include "environment.h"


#ifdef __cplusplus
extern "C" {
#endif

int equal_reprtab(int i, int j, environment *const env);
void output_keywords(environment *const env);
int macro_keywords(environment *const env);
int collect_mident(environment *const env);

/**
 *	Skip all spaces and tabs to the end of the line
 *
 *	@param	env	Preprocessor environment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int skip_line(environment *const env);

/**
 *	Skip all spaces and tabs up to a significant character
 *
 *	@param	env	Preprocessor environment
 */
void skip_separators(environment *const env);

/**
 *	Skip all characters inside quotes
 *
 *	@param	env	Preprocessor environment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int skip_string(environment *const env);

/**
 *	Assigning the current character EOF
 *
 *	@param	env	Preprocessor environment
 */
void end_of_file(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
