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


#define LOGIC      0
#define ARITHMETIC 1


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Calculate an arithmetic or logical expression
 *
 *	@param	env				Preprocessor environment
 *	@param	type			Type expression
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int calculate(environment *const env, const int type);

#ifdef __cplusplus
} /* extern "C" */
#endif
