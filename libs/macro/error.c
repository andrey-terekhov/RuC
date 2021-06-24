/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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


#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stddef.h>


#define TAG_MACRO		"macro"

#define ERROR_TAG_SIZE	256
#define ERROR_MSG_SIZE	256


void get_message_error(const int num, char *const msg)
{
	switch (num)
	{
		case source_file_not_found:
			sprintf(msg, "исходный файл не найден");
			break;
		case include_file_not_found:
			sprintf(msg, "заголовочный файл не найден");
			break;
		default:
			sprintf(msg, "не реализованная ошибка №%d", num);
			break;
	}
}

void macro_system_error(const char *const tag, const int num)
{
	char msg[ERROR_MSG_SIZE];
	get_message_error(num, msg);

	if (tag != NULL)
	{
		log_system_error(tag, msg);
	}
	else
	{
		log_system_error(TAG_MACRO, msg);
	}
}
