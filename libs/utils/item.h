/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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
#include <stdint.h>
#include "dll.h"
#include "workspace.h"


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

/** Item type for output tables */
typedef enum ITEM_STATUS
{
	item_error = -1,		/**< Error code */
	item_int64,				/**< Item is int64_t */
	item_int32,				/**< Item is int32_t */
	item_int16,				/**< Item is int16_t */
	item_int8,				/**< Item is int8_t */
	item_uint64,			/**< Item is uint64_t */
	item_uint32,			/**< Item is uint32_t */
	item_uint16,			/**< Item is uint16_t */
	item_uint8,				/**< Item is uint8_t */
	item_types,				/**< Max item types */
} item_status;

/** Item type */
typedef ITEM item_t;


/**
 *	Get target item type from workspace flags
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Item status
 */
EXPORTED item_status item_get_status(const workspace *const ws);

/**
 *	Get target item min
 *
 *	@param	status		Item status
 *
 *	@return	Target @c ITEM_MIN
 */
EXPORTED item_t item_get_min(const item_status status);

/**
 *	Get target item max
 *
 *	@param	status		Item status
 *
 *	@return	Target @c ITEM_MAX
 */
EXPORTED item_t item_get_max(const item_status status);

/**
 *	Check that variable is not out of range
 *
 *	@param	status		Item status
 *	@param	var			Checking variable
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED int item_check_var(const item_status status, const item_t var);

#ifdef __cplusplus
} /* extern "C" */
#endif
