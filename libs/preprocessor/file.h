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

#include "environment.h"
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Length of char32_t string
 *
 *	@param	str		String
 *
 *	@return	Length of string, @c SIZE_MAX on failure
 */
size_t strlen32(const char32_t *const str);

void m_change_nextch_type(int type, int p, environment *const env);
void m_old_nextch_type(environment *const env);

int get_dipp(environment *const env);
int get_next_char(environment *const env);

void m_fprintf(int a, environment *const env);
void pred_fprintf(int a, environment *const env);
void m_nextch(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
