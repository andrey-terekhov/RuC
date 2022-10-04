/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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

#include "macro.h"
#include "linker.h"
#include "parser.h"
#include "storage.h"
#include "uniio.h"
#include "uniprinter.h"


static const size_t OUT_BUFFER_SIZE = 1024;


static inline size_t ws_parse_name(const char *const name, char32_t *const buffer)
{
	buffer[0] = utf8_convert(&name[0]);
	if (buffer[0] == '\0' || buffer[0] == '=')
	{
		macro_system_error(NULL, MACRO_NAME_NON);
		return SIZE_MAX;
	}
	else if (!utf8_is_letter(buffer[0]))
	{
		macro_system_error(NULL, MACRO_NAME_FIRST_CHARACTER);
		return SIZE_MAX;
	}

	size_t i = 0;
	size_t j = utf8_size(buffer[0]);
	do
	{
		buffer[++i] = utf8_convert(&name[j]);
		j += utf8_size(buffer[i]);
	} while (utf8_is_letter(buffer[i]) || utf8_is_digit(buffer[i]));

	if (buffer[i] == '\0')
	{
		return j - 1;
	}
	else if (buffer[i] == '=')
	{
		buffer[i] = '\0';
		return j;
	}
	else
	{
		macro_system_warning(NULL, MACRO_CONSOLE_SEPARATOR);
		j -= utf8_size(buffer[i]);
		buffer[i] = '\0';
		return j;
	}
}

static inline void ws_parse_value(const char *const value, char32_t *const buffer)
{
	size_t i = 0;
	size_t j = 0;
	do
	{
		buffer[i] = utf8_convert(&value[j]);
		j += utf8_size(buffer[i]);
	} while (buffer[i++] != '\0');
}

static inline int ws_parse(const workspace *const ws, storage *const stg)
{
	for (size_t i = 0; i < ws_get_flags_num(ws); i++)
	{
#ifdef _WIN32
		char flag[MAX_ARG_SIZE];
		utf8_from_cp1251(ws_get_flag(ws, i), flag);
#else
		const char *flag = ws_get_flag(ws, i);
#endif

		if (flag[0] == '-' && flag[1] == 'D')
		{
			char32_t name[MAX_ARG_SIZE];

			const size_t index = ws_parse_name(&flag[2], name);
			if (index == SIZE_MAX)
			{
				return -1;
			}

			if (flag[2 + index] != '\0')
			{
				char32_t value[MAX_ARG_SIZE];
				ws_parse_value(&flag[2 + index], value);
				if (storage_add_utf8(stg, name, value) == SIZE_MAX)
				{
					macro_system_error(NULL, MACRO_NAME_EXISTS, name);
					return -1;
				}
			}
			else if (storage_add_utf8(stg, name, NULL) == SIZE_MAX)
			{
				macro_system_error(NULL, MACRO_NAME_EXISTS, name);
				return -1;
			}
		}
	}

	return 0;
}


static int macro_form_io(workspace *const ws, universal_io *const output)
{
	linker lk = linker_create(ws);
	storage stg = storage_create();
	parser prs = parser_create(&lk, &stg, output);

	int ret = ws_parse(ws, &stg);

	const size_t size = linker_size(&lk);
	for (size_t i = 0; i < size && !ret; i++)
	{
		universal_io in = linker_add_source(&lk, i);
		if (!in_is_correct(&in))
		{
			macro_system_error(TAG_LINKER, LINKER_CANNOT_OPEN);
		}

		ret = parser_preprocess(&prs, &in);
		in_clear(&in);
	}

	parser_clear(&prs);
	storage_clear(&stg);
	linker_clear(&lk);
	return ret;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


char *macro(workspace *const ws)
{
	if (ws_get_files_num(ws) == 0)
	{
		return NULL;
	}

	universal_io io = io_create();
	if (out_set_buffer(&io, OUT_BUFFER_SIZE))
	{
		return NULL;
	}

	int ret = macro_form_io(ws, &io);
	if (ret)
	{
		io_erase(&io);
		return NULL;
	}

	in_clear(&io);
	return out_extract_buffer(&io);
}

int macro_to_file(workspace *const ws, const char *const path)
{
	if (ws_get_files_num(ws) == 0)
	{
		return -1;
	}

	universal_io io = io_create();
	if (out_set_file(&io, path))
	{
		return -1;
	}

	int ret = macro_form_io(ws, &io);

	io_erase(&io);
	return ret;
}


char *auto_macro(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	char *ret = macro(&ws);
	ws_clear(&ws);
	return ret;
}

int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path)
{
	workspace ws = ws_parse_args(argc, argv);
	const int ret = macro_to_file(&ws, path);
	ws_clear(&ws);
	return ret;
}
