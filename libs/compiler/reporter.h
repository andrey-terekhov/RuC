/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "errors.h"
#include "token.h"
#include "workspace.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Reporter */
typedef struct reporter
{
	size_t errors;							/**< Number of reported errors */
	size_t warnings;						/**< Number of reported warnings */

	bool is_recovery_disabled;				/**< Set, if error recovery & multiple output disabled */
} reporter;


/**
 *	Create reporter
 *
 *	@param	ws		Compiler workspace
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
reporter reporter_create(const workspace *const ws);

/**
 *	Get reported error number
 *
 *	@param	rprt		Reporter
 */
size_t reporter_get_errors_number(reporter *const rprt);

/**
 *	Report an error
 *
 *	@param	rprt		Reporter
 *	@param	io			Universal io
 *	@param	loc			Error location
 *	@param	num			Error code
 *	@param	args		Variable list
 */
void report_error(reporter *const rprt, universal_io *const io, const location loc, const err_t num, va_list args);

/**
 *	Report a warning
 *
 *	@param	rprt		Reporter
 *	@param	io			Universal io
 *	@param	loc			Warning location	
 *	@param	num			Warning code
 *	@param	args		Variable list
 */
void report_warning(reporter *const rprt, universal_io *const io, const location loc, const warning_t num, va_list args);

#ifdef __cplusplus
} /* extern "C" */
#endif
