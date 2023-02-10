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

	CALL_DEPTH,
	ITERATION_MAX,
	STRING_UNTERMINATED,
	COMMENT_UNTERMINATED,

	DIRECTIVE_NAME_NON,
	DIRECTIVE_INVALID,
	DIRECTIVE_FORBIDDEN,
	DIRECTIVE_UNTERMINATED,
	DIRECTIVE_NO_EXPRESSION,
	DIRECTIVE_WITHOUT,
	DIRECTIVE_AFTER,

	HASH_STRAY,
	HASH_ON_EDGE,
	HASH_NOT_FOLLOWED,

	INCLUDE_EXPECTS_FILENAME,
	INCLUDE_NO_SUCH_FILE,
	INCLUDE_DEPTH,

	ARGS_DUPLICATE,
	ARGS_EXPECTED_NAME,
	ARGS_EXPECTED_COMMA,
	ARGS_EXPECTED_BRACKET,
	ARGS_UNTERMINATED,
	ARGS_REQUIRES,
	ARGS_PASSED,
	ARGS_NON,

	EXPR_FLOATING_CONSTANT,
	EXPR_INVALID_SUFFIX,
	EXPR_INVALID_TOKEN,
	EXPR_MISSING_BINARY,
	EXPR_MISSING_BRACKET,
	EXPR_MISSING_BETWEEN,
	EXPR_NO_LEFT_OPERAND,
	EXPR_NO_RIGHT_OPERAND,
} error_t;

/** Warning codes */
typedef enum WARNING
{
	MACRO_CONSOLE_SEPARATOR,
	MACRO_NAME_UNDEFINED,
	MACRO_NAME_REDEFINE,

	DIRECTIVE_EXTRA_TOKENS,
	DIRECTIVE_LINE_SKIPED,

	EXPR_MULTI_CHARACTER,
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
