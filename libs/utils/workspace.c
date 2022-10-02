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

#include "workspace.h"
#include <string.h>

#ifndef _WIN32
	#include <unistd.h>
#else
	#define F_OK 0

	extern int access(const char *path, int mode);
#endif


static const size_t MAX_FLAGS = 32;


typedef size_t (*ws_add)(workspace *const ws, const char *const str);


static inline void ws_add_error(workspace *const ws)
{
	if (ws != NULL)
	{
		ws->was_error = true;
	}
}

static inline int ws_add_array(workspace *const ws, const ws_add func, const char *const *const arr, const size_t num)
{
	if (arr == NULL)
	{
		ws_add_error(ws);
		return -1;
	}

	for (size_t i = 0; i < num; i++)
	{
		if (func(ws, arr[i]) == SIZE_MAX)
		{
			return -1;
		}
	}
	return 0;
}

static void ws_unix_path(const char *const path, char *const buffer)
{
	size_t i = 0;
	size_t j = 0;
	size_t last = 0;

	while (path[i] != '\0')
	{
		buffer[j++] = path[i] == '\\' ? '/' : path[i];

		if (buffer[j - 1] == '/')
		{
			if (j - last == 2 && buffer[last] == '.')
			{
				j = last;
			}
			else if (last != 0 && j - last == 3 && buffer[last] == '.' && buffer[last + 1] == '.'
				&& !((last > 3 && buffer[last - 2] == '.' && buffer[last - 3] == '.' && buffer[last - 4] == '/')
					|| (last == 3 && buffer[last - 2] == '.' && buffer[last - 3] == '.')))
			{
				j = last - 1;
				while (j > 0 && buffer[j - 1] != '/')
				{
					j--;
				}
			}

			last = j;
		}

		i++;
	}

	buffer[buffer[j - 1] == '/' ? j - 1 : j] = '\0';
}

static size_t ws_exists(const char *const element, const strings *const vec)
{
	for (size_t i = 0; i < strings_size(vec); i++)
	{
		if (strcmp(element, strings_get(vec, i)) == 0)
		{
			return i;
		}
	}

	return SIZE_MAX;
}

static inline size_t ws_add_string(strings *const vec, const char *const str)
{
	const size_t index = ws_exists(str, vec);
	if (index != SIZE_MAX)
	{
		return index;
	}

	return strings_add(vec, str);
}

static inline size_t ws_add_path(workspace *const ws, strings *const vec, const char *const path)
{
	if (!ws_is_correct(ws) || path == NULL)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	char buffer[MAX_ARG_SIZE];
	ws_unix_path(path, buffer);
	if (access(buffer, F_OK) == -1)
	{
		ws->was_error = true;
		return SIZE_MAX;
	}

	return ws_add_string(vec, buffer);
}

static inline bool ws_is_dir_flag(const char *const flag)
{
	return flag[0] == '-' && flag[1] == 'I';
}

static inline size_t ws_get_num(const strings *const vec)
{
	const size_t size = strings_size(vec);
	return size != SIZE_MAX ? size : 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


workspace ws_parse_args(const int argc, const char *const *const argv)
{
	workspace ws = ws_create();

	if (argv == NULL)
	{
		ws.was_error = true;
		return ws;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i] == NULL)
		{
			ws.was_error = true;
			return ws;
		}

		if (argv[i][0] != '-')
		{
			if (ws_add_file(&ws, argv[i]) == SIZE_MAX)
			{
				return ws;
			}
			continue;
		}

		if (argv[i][1] == 'o' && argv[i][2] == '\0')
		{
			if (i + 1 >= argc || ws_set_output(&ws, argv[++i]) == -1)
			{
				ws.was_error = true;
				return ws;
			}
			continue;
		}

		if (ws_add_flag(&ws, argv[i]) == SIZE_MAX)
		{
			return ws;
		}
	}

	return ws;
}


workspace ws_create(void)
{
	workspace ws;

	ws.files = strings_create(MAX_PATHS);
	ws.dirs = strings_create(MAX_PATHS);
	ws.flags = strings_create(MAX_FLAGS);

	ws.output[0] = '\0';
	ws.was_error = false;

	return ws;
}


size_t ws_add_file(workspace *const ws, const char *const path)
{
	return ws_add_path(ws, &ws->files, path);
}

int ws_add_files(workspace *const ws, const char *const *const paths, const size_t num)
{
	return ws_add_array(ws, &ws_add_file, paths, num);
}


size_t ws_add_dir(workspace *const ws, const char *const path)
{
	return ws_add_path(ws, &ws->dirs, path);
}

int ws_add_dirs(workspace *const ws, const char *const *const paths, const size_t num)
{
	return ws_add_array(ws, &ws_add_dir, paths, num);
}


size_t ws_add_flag(workspace *const ws, const char *const flag)
{
	if (!ws_is_correct(ws) || flag == NULL)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	return ws_is_dir_flag(flag) ? ws_add_dir(ws, &flag[2]) : ws_add_string(&ws->flags, flag);
}

int ws_add_flags(workspace *const ws, const char *const *const flags, const size_t num)
{
	return ws_add_array(ws, &ws_add_flag, flags, num);
}


int ws_set_output(workspace *const ws, const char *const path)
{
	if (!ws_is_correct(ws) || path == NULL)
	{
		ws_add_error(ws);
		return -1;
	}

	strcpy(ws->output, path);
	return 0;
}


bool ws_is_correct(const workspace *const ws)
{
	return ws != NULL && !ws->was_error;
}


const char *ws_get_file(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) ? strings_get(&ws->files, index) : NULL;
}

size_t ws_get_files_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws_get_num(&ws->files) : 0;
}

const char *ws_get_dir(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) ? strings_get(&ws->dirs, index) : NULL;
}

size_t ws_get_dirs_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws_get_num(&ws->dirs) : 0;
}

const char *ws_get_flag(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) ? strings_get(&ws->flags, index) : NULL;
}

size_t ws_get_flags_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws_get_num(&ws->flags) : 0;
}


const char *ws_get_output(const workspace *const ws)
{
	return ws_is_correct(ws) && ws->output[0] != '\0' ? ws->output : NULL;
}


int ws_clear(workspace *const ws)
{
	if (ws == NULL)
	{
		return -1;
	}

	strings_clear(&ws->files);
	strings_clear(&ws->dirs);
	strings_clear(&ws->flags);

	ws->was_error = true;
	return 0;
}
