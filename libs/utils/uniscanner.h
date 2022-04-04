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

#include <stdio.h>
#include "dll.h"
#include "uniio.h"


#define MAX_SYMBOL_SIZE 8


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Universal scanf-like function
 *
 *	@param	io			Universal io structure
 *	@param	format		String format
 *
 *	@return	Return scanf-like value
 */
EXPORTED int uni_scanf(universal_io *const io, const char *const format, ...)
	__attribute__((format(scanf, 2, 3)))
	__attribute__((nonnull(2)));

/**
 *	Universal function for scanning UTF-8 characters
 *
 *	@param	io			Universal io structure
 *
 *	@return	UTF-8 character
 */
EXPORTED char32_t uni_scan_char(universal_io *const io);

/**
 *	Universal function for discarding UTF-8 characters
 *
 *	@param	io			Universal io structure
 *	@param	wchar		UTF-8 character
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int uni_unscan_char(universal_io *const io, const char32_t wchar);

#ifdef __cplusplus
} /* extern "C" */
#endif
