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

#include "item.h"
#include <math.h>
#include <string.h>
#include "workspace.h"


#ifndef abs
	#define abs(a) ((a) < 0 ? -(a) : (a))
#endif


#if ITEM > 32
	static const item_status CURRENT_STATUS = item_uint64;
#elif ITEM > 16
	static const item_status CURRENT_STATUS = item_uint32;
#elif ITEM > 8
	static const item_status CURRENT_STATUS = item_uint16;
#elif ITEM >= 0
	static const item_status CURRENT_STATUS = item_uint8;
#elif ITEM >= -8
	static const item_status CURRENT_STATUS = item_int8;
#elif ITEM >= -16
	static const item_status CURRENT_STATUS = item_int16;
#elif ITEM >= -32
	static const item_status CURRENT_STATUS = item_int32;
#else
	static const item_status CURRENT_STATUS = item_int64;
#endif

static const item_status DEFAULT_STATUS = item_int32;


static inline size_t item_get_size(const item_status status)
{
	if (status <= item_error || status >= item_types)
	{
		return 0;
	}

	size_t size = status == item_int64 || status == item_uint64
					? 1
					: status == item_int32 || status == item_uint32
						? 2
						: status == item_int16 || status == item_uint16
							? 4
							: 8;

	if ((size_t)abs(ITEM) <= 32 / size)
	{
		size = (size_t)pow(2, ceil(log2(abs(ITEM))));
	}

	return size;
}

static inline size_t item_get_shift(const size_t size)
{
	return 64 / size;
}

static inline uint64_t item_get_mask(const size_t shift)
{
	uint64_t mask = 0x00000000000000FF;
	for (size_t i = 8; i < shift; i *= 2)
	{
		mask = (mask << i) | mask;
	}

	return mask;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


item_status item_get_status(const workspace *const ws)
{
	if (!ws_is_correct(ws))
	{
		return item_error;
	}

	item_status status = item_types;

	size_t i = 0;
	const char *flag = ws_get_flag(ws, i);
	while (flag != NULL && status != item_error)
	{
		if (strcmp(flag, "-i64") == 0)
		{
			status = status != item_types ? item_error : item_int64;
		}
		else if (strcmp(flag, "-i32") == 0)
		{
			status = status != item_types ? item_error : item_int32;
		}
		else if (strcmp(flag, "-i16") == 0)
		{
			status = status != item_types ? item_error : item_int16;
		}
		else if (strcmp(flag, "-i8") == 0)
		{
			status = status != item_types ? item_error : item_int8;
		}
		else if (strcmp(flag, "-u64") == 0)
		{
			status = status != item_types ? item_error : item_uint64;
		}
		else if (strcmp(flag, "-u32") == 0)
		{
			status = status != item_types ? item_error : item_uint32;
		}
		else if (strcmp(flag, "-u16") == 0)
		{
			status = status != item_types ? item_error : item_uint16;
		}
		else if (strcmp(flag, "-u8") == 0)
		{
			status = status != item_types ? item_error : item_uint8;
		}

		flag = ws_get_flag(ws, ++i);
	}

	return status == item_types ? DEFAULT_STATUS : status;
}

item_t item_get_min(const item_status status)
{
	switch (status)
	{
		case item_int8:
#if ITEM_MIN <= CHAR_MIN
			return CHAR_MIN;
#endif
		case item_int16:
#if ITEM_MIN <= SHRT_MIN
			return SHRT_MIN;
#endif
		case item_int32:
#if ITEM_MIN <= INT_MIN
			return INT_MIN;
#endif
		case item_int64:
#if ITEM_MIN <= LLONG_MIN
			return LLONG_MIN;
#else
			return ITEM_MIN;
#endif

		case item_uint8:
		case item_uint16:
		case item_uint32:
		case item_uint64:

		default:
			return 0;
	}
}

item_t item_get_max(const item_status status)
{
	switch (status)
	{
		case item_int8:
#if ITEM_MAX >= CHAR_MAX
			return CHAR_MAX;
#endif
		case item_uint8:
#if ITEM_MAX >= UCHAR_MAX
			return UCHAR_MAX;
#endif

		case item_int16:
#if ITEM_MAX >= SHRT_MAX
			return SHRT_MAX;
#endif
		case item_uint16:
#if ITEM_MAX >= USHRT_MAX
			return USHRT_MAX;
#endif

		case item_int32:
#if ITEM_MAX >= INT_MAX
			return INT_MAX;
#endif
		case item_uint32:
#if ITEM_MAX >= UINT_MAX
			return UINT_MAX;
#endif

		case item_int64:
#if ITEM_MAX >= LLONG_MAX
			return LLONG_MAX;
#endif
		case item_uint64:
#if ITEM_MAX >= ULLONG_MAX
			return ULLONG_MAX;
#else
			return ITEM_MAX;
#endif

		default:
			return 0;
	}
}


size_t item_store_double(const double value, item_t *const stg)
{
	return item_store_double_for_target(CURRENT_STATUS, value, stg);
}

size_t item_store_double_for_target(const item_status status, const double value, item_t *const stg)
{
	int64_t temp = 0;
	memcpy(&temp, &value, sizeof(double));
	return item_store_int64_for_target(status, temp, stg);
}

double item_restore_double(const item_t *const stg)
{
	return item_restore_double_for_target(CURRENT_STATUS, stg);
}

double item_restore_double_for_target(const item_status status, const item_t *const stg)
{
	const int64_t temp = item_restore_int64_for_target(status, stg);
	if (temp == LLONG_MAX)
	{
		return DBL_MAX;
	}

	double value;
	memcpy(&value, &temp, sizeof(double));
	return value;
}


size_t item_store_int64(const int64_t value, item_t *const stg)
{
	return item_store_int64_for_target(CURRENT_STATUS, value, stg);
}

size_t item_store_int64_for_target(const item_status status, const int64_t value, item_t *const stg)
{
	const size_t size = item_get_size(status);
	if (stg == NULL || size == 0)
	{
		return SIZE_MAX;
	}

	const size_t shift = item_get_shift(size);
	const uint64_t mask = item_get_mask(shift);
	const uint64_t sign = status == item_int64 || status == item_int32
							|| status == item_int16 || status == item_int8
								? (~mask >> 1) & mask  : 0;

	uint64_t temp = mask;
	for (size_t i = 0; i < size; i++)
	{
		stg[i] = (item_t)(((uint64_t)value & temp) >> (shift * i));
		if ((uint64_t)stg[i] & sign)
		{
			stg[i] |= ~mask;
		}

		temp <<= shift;
	}

	return size;
}

int64_t item_restore_int64(const item_t *const stg)
{
	return item_restore_int64_for_target(CURRENT_STATUS, stg);
}

int64_t item_restore_int64_for_target(const item_status status, const item_t *const stg)
{
	const size_t size = item_get_size(status);
	if (stg == NULL || size == 0)
	{
		return LLONG_MAX;
	}

	const size_t shift = item_get_shift(size);
	const uint64_t mask = item_get_mask(shift);

	int64_t value = 0;
	for (size_t i = 0; i < size; i++)
	{
		value |= ((uint64_t)stg[i] & mask) << (shift * i);
	}

	return value;
}


bool item_check_var(const item_status status, const item_t var)
{
	return var >= item_get_min(status) && var <= item_get_max(status);
}
