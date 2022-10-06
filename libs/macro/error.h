/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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

#include <stdarg.h>
#include "locator.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Error codes */
typedef enum ERROR
{
	LINKER_NO_INPUT,
	LINKER_WRONG_IO,
	LINKER_CANNOT_OPEN,

	MACRO_NAME_NON,
	MACRO_NAME_FIRST_CHARACTER,
	MACRO_NAME_EXISTS,

	PARSER_MISSING_TERMINATION,
} error_t;

/** Warning codes */
typedef enum WARNING
{
	MACRO_CONSOLE_SEPARATOR,
} warning_t;


/**
 *	Emit an error for some problem
 *
 *	@param	loc			Emitted location
 *	@param	num			Error code
 */
void macro_error(location *const loc, error_t num, ...);

/**
 *	Emit a warning for some problem
 *
 *	@param	loc			Emitted location
 *	@param	num			Warning code
 */
void macro_warning(location *const loc, warning_t num, ...);


/**
 *	Emit an error (embedded version)
 *
 *	@param	loc			Emitted location
 *	@param	num			Error code
 *	@param	args		Variable list
 */
void macro_verror(location *const loc, const error_t num, va_list args);

/**
 *	Emit a warning (embedded version)
 *
 *	@param	loc			Emitted location
 *	@param	num			Warning code
 *	@param	args		Variable list
 */
void macro_vwarning(location *const loc, const warning_t num, va_list args);


/**
 *	Emit a system error
 *
 *	@param	tag		Message location
 *	@param	num		Error code
 */
void macro_system_error(const char *const tag, error_t num, ...);

/**
 *	Emit a system warning
 *
 *	@param	tag		Message location
 *	@param	num		Warning code
 */
void macro_system_warning(const char *const tag, warning_t num, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
