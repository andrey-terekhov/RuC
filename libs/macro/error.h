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

#include <stddef.h>
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Errors codes */
typedef enum ERROR
{
	include_file_not_found = 365,
	source_file_not_found,
} error_t;


/**
 *	Emit an error for some problem
 *
 *	@param	io			Universal io
 *	@param	num			Error code
 */
void macro_error(const universal_io *const io, const int num);

/**
 *	Emit a warning for some problem
 *
 *	@param	io			Universal io
 *	@param	num			Warning code
 */
void macro_warning(const universal_io *const io, const int num);


/**
 *	Emit an error message
 *
 *	@param	io			Universal io
 *	@param	msg			Error message
 */
void macro_error_msg(const universal_io *const io, const char *const msg);

/**
 *	Emit a warning message
 *
 *	@param	io			Universal io
 *	@param	msg			Warning message
 */
void macro_warning_msg(const universal_io *const io, const char *const msg);


/**
 *	Emit a system error
 *
 *	@param	tag		Message location
 *	@param	num		Error code
 */
void macro_system_error(const char *const tag, error_t num);

/**
 *	Emit a system warning
 *
 *	@param	tag		Message location
 *	@param	num		Warning code
 */
void macro_system_warning(const char *const tag, error_t num);

#ifdef __cplusplus
} /* extern "C" */
#endif
