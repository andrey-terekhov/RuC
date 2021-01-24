/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev
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

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "dll.h"


#define DEFAULT_ITEM int64_t
#define DEFAULT_MIN LLONG_MIN
#define DEFAULT_MAX LLONG_MAX

#ifndef ITEM
	#define ITEM DEFAULT_ITEM
	#define ITEM_MIN DEFAULT_MIN
	#define ITEM_MAX DEFAULT_MAX
#elif ITEM == int64_t
	#define ITEM_MIN LLONG_MIN
	#define ITEM_MAX LLONG_MAX
#elif ITEM == int32_t
	#define ITEM_MIN INT_MIN
	#define ITEM_MAX INT_MAX
#elif ITEM == int16_t
	#define ITEM_MIN SHRT_MIN
	#define ITEM_MAX SHRT_MAX
#elif ITEM == int8_t
	#define ITEM_MIN CHAR_MIN
	#define ITEM_MAX CHAR_MAX
#elif ITEM == uint64_t
	#define ITEM_MIN 0
	#define ITEM_MAX ULLONG_MAX
#elif ITEM == uint32_t
	#define ITEM_MIN 0
	#define ITEM_MAX UINT_MAX
#elif ITEM == uint16_t
	#define ITEM_MIN 0
	#define ITEM_MAX USHRT_MAX
#elif ITEM == uint8_t
	#define ITEM_MIN 0
	#define ITEM_MAX UCHAR_MAX
#else
	#undef ITEM
	#define ITEM DEFAULT_ITEM
	#define ITEM_MIN DEFAULT_MIN
	#define ITEM_MAX DEFAULT_MAX
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef ITEM item_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
