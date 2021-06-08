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
#include <stddef.h>
#include <string.h>


const item_status DEFAULT_STATUS = item_int32;


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

bool item_check_var(const item_status status, const item_t var)
{
	return var >= item_get_min(status) && var <= item_get_max(status);
}
