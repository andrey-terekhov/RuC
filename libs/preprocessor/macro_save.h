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


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Add new macro
 *
 *	@param	env	Preprocessor environment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int macro_add(environment *const env);

/**
 *	Modify an existing macro
 *
 *	@param	env	Preprocessor environment
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int macro_set(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
