/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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
#include "dll.h"
#include "locator.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Prototype of custom logging function
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 */
typedef void (*logger)(const char *const tag, const char *const msg);


/**
 *	Set custom error logging function
 *
 *	@param	func	Custom logging function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int set_error_log(const logger func);

/**
 *	Set custom warning logging function
 *
 *	@param	func	Custom logging function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int set_warning_log(const logger func);

/**
 *	Set custom note logging function
 *
 *	@param	func	Custom logging function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int set_note_log(const logger func);


/**
 *	Add error message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 *	@param	line	Code line
 *	@param	symbol	Position in line, if more than length, then turn off highlighting
 */
EXPORTED void log_error(const char *const tag, const char *const msg, const char *const line, const size_t symbol);

/**
 *	Add warning message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 *	@param	line	Code line
 *	@param	symbol	Position in line, if more than length, then turn off highlighting
 */
EXPORTED void log_warning(const char *const tag, const char *const msg, const char *const line, const size_t symbol);

/**
 *	Add note message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 *	@param	line	Code line
 *	@param	symbol	Position in line, if more line length, then turn off highlighting
 */
EXPORTED void log_note(const char *const tag, const char *const msg, const char *const line, const size_t symbol);


/**
 *	Add error message to log
 *
 *	@param	loc		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_auto_error(location *const loc, const char *const msg);

/**
 *	Add warning message to log
 *
 *	@param	loc		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_auto_warning(location *const loc, const char *const msg);

/**
 *	Add note message to log
 *
 *	@param	loc		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_auto_note(location *const loc, const char *const msg);


/**
 *	Add error message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_system_error(const char *const tag, const char *const msg);

/**
 *	Add warning message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_system_warning(const char *const tag, const char *const msg);

/**
 *	Add note message to log
 *
 *	@param	tag		Message location
 *	@param	msg		Message content
 */
EXPORTED void log_system_note(const char *const tag, const char *const msg);

#ifdef __cplusplus
} /* extern "C" */
#endif
