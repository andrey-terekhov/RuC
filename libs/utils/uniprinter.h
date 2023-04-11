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

#include "dll.h"
#include "uniio.h"
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Universal printf-like function
 *
 *	@param	io			Universal io structure
 *	@param	format		String format
 *
 *	@return	Return printf-like value
 */
// clang-format off
EXPORTED int uni_printf(universal_io *const io, const char *const format, ...)
	__attribute__((format(printf, 2, 3)))
	__attribute__((nonnull(2)));
// clang-format on

/**
 *	Universal function for printing UTF-8 characters
 *
 *	@param	io			Universal io structure
 *	@param	wchar		UTF-8 character
 *
 *	@return	Return printf-like value
 */
EXPORTED int uni_print_char(universal_io *const io, const char32_t wchar);

#ifdef __cplusplus
} /* extern "C" */
#endif
