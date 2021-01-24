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
		if (strcmp(flag, "-int64") == 0 || strcmp(flag, "-INT64") == 0)
		{
			status = status != item_types ? item_error : item_int64;
		}
		else if (strcmp(flag, "-int32") == 0 || strcmp(flag, "-INT32") == 0)
		{
			status = status != item_types ? item_error : item_int32;
		}
		else if (strcmp(flag, "-int16") == 0 || strcmp(flag, "-INT16") == 0)
		{
			status = status != item_types ? item_error : item_int16;
		}
		else if (strcmp(flag, "-int8") == 0 || strcmp(flag, "-INT8") == 0)
		{
			status = status != item_types ? item_error : item_int8;
		}
		else if (strcmp(flag, "-uint64") == 0 || strcmp(flag, "-UINT64") == 0)
		{
			status = status != item_types ? item_error : item_uint64;
		}
		else if (strcmp(flag, "-uint32") == 0 || strcmp(flag, "-UINT32") == 0)
		{
			status = status != item_types ? item_error : item_uint32;
		}
		else if (strcmp(flag, "-uint16") == 0 || strcmp(flag, "-UINT16") == 0)
		{
			status = status != item_types ? item_error : item_uint16;
		}
		else if (strcmp(flag, "-uint8") == 0 || strcmp(flag, "-UINT8") == 0)
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
		case item_int64:
			return LLONG_MIN;
		case item_int32:
			return INT_MIN;
		case item_int16:
			return SHRT_MIN;
		case item_int8:
			return CHAR_MIN;

		case item_uint64:
		case item_uint32:
		case item_uint16:
		case item_uint8:
		
		default:
			return 0;
	}
}

item_t item_get_max(const item_status status);

int item_check_var(const item_status status, const item_t var);
