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

#include "context.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Emit an error for some problem
 *
 *	@param	io		Universal io
 *	@param	num		Error number
 */
void error(compiler_context *const context, const int num);

/**
 *	Emit a warning for some problem
 *
 *	@param	io		Universal io
 *	@param	num		Warning number
 */
void warning(compiler_context *const context, const int num);


/**
 *	Emit error by number
 *
 *	@param	num		Error number
 */
void system_error(const int num);

/**
 *	Emit warning by number
 *
 *	@param	num		Warning number
 */
void system_warning(const int num);


/**
 *	Emit error message
 *
 *	@param	msg		Error message
 */
void system_error_msg(const char *const msg);

/**
 *	Emit warning message
 *
 *	@param	msg		Warning message
 */
void system_warning_msg(const char *const msg);

#ifdef __cplusplus
} /* extern "C" */
#endif
