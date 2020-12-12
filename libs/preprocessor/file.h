/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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

int strlen32(char32_t* strarg);
void m_change_nextch_type(int type, int p, preprocess_context *context);
void m_old_nextch_type(preprocess_context *context);

int get_dipp(preprocess_context *context);
int get_next_char(preprocess_context *context);

void m_fprintf(int a, preprocess_context *context);
void pred_fprintf(int a, preprocess_context *context);
void m_nextch(preprocess_context *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
