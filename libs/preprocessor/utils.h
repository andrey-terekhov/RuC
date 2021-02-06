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
void output_keywods(environment *const env);
int macro_keywords(environment *const env);
int collect_mident(environment *const env);
int find_file(environment *const env, const char *s);

int space_end_line(environment *const env);
void skip_space(environment *const env);
void skip_space_str(environment *const env);
size_t skip_str(environment *const env);
void skip_file(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
