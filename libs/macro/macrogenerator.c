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

#include "macrogenerator.h"
#include "linker.h"
#include "parser.h"
#include "uniio.h"
#include "uniprinter.h"


const size_t SIZE_OUT_BUFFER = 1024;

static int macro_form_io(workspace *const ws, universal_io *const output)
{
	linker lk = linker_create(ws);
	parser prs = parser_create(&lk, output);
	size_t file_size = linker_size(&lk);

	for (size_t i = 0; i < file_size; i++)
	{
		universal_io in = linker_add_source(&lk, i);
		if (!in_is_correct(&in))
		{
			linker_clear(&lk);
			return -1;
		}

		if (parser_preprocess(&prs, &in))
		{
			linker_clear(&lk);
			return -1;
		}

		in_clear(&in);
	}

	linker_clear(&lk);
	return 0;
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
	if (!ws_is_correct(ws) || ws_get_files_num(ws) == 0)
	{
		return NULL;
	}

	universal_io io = io_create();
	if (out_set_buffer(&io, SIZE_OUT_BUFFER))
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
	if (!ws_is_correct(ws) || ws_get_files_num(ws) == 0)
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
	return macro(&ws);
}

int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path)
{
	workspace ws = ws_parse_args(argc, argv);
	return macro_to_file(&ws, path);
}
