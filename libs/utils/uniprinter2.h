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
#include "io2.h"
#include "utils_internal.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Parse command line arguments
 *
 *	@param	io			Command line arguments
 *	@param	format		Command line arguments
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int uni_printf(universal_io *const io, const char *const format, ...);

/**
 *	Parse command line arguments
 *
 *	@param	io			Command line arguments
 *	@param	wchar		Command line arguments
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int uni_print_char(universal_io *const io, const char32_t wchar);

#ifdef __cplusplus
} /* extern "C" */
#endif
