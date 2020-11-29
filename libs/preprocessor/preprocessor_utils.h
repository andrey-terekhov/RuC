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

#include "context_var.h"


#ifdef __cplusplus
extern "C" {
#endif

int strlen32(const char32_t* strarg);
void output_keywods(preprocess_context *context);
void collect_mident(preprocess_context *context, char32_t* str);

void space_end_line(preprocess_context *context);
void space_skip(preprocess_context *context);
void space_skip_str(preprocess_context *context);

int is_letter(preprocess_context *context);
int is_digit(int a);

#ifdef __cplusplus
} /* extern "C" */
#endif
