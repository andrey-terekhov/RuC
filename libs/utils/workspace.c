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
#include <stdlib.h>

#ifndef _MSC_VER
	#include <unistd.h>
#else
	#define F_OK 0

	extern int access(const char *path, int mode);
#endif


const char *const DEFAULT_OUTPUT = "export.txt";


void ws_init(workspace *const ws)
{
	ws->files_num = 0;
	ws->dirs_num = 0;
	ws->flags_num = 0;

	strcpy(ws->output, DEFAULT_OUTPUT);
	ws->was_error = 0;
}

void ws_add_error(workspace *const ws)
{
	if (ws == NULL)
	{
		return;
	}

	ws->was_error = 1;
}

static int ws_unix_path(const char *const path, char *const buffer, const size_t max_len)
{
	char *tmp;
#ifndef _WIN32
	char  resolved[PATH_MAX];

	tmp = realpath(path, resolved);
	if (tmp == NULL || strlen(resolved) >= max_len)
		return -1;
	strcpy(buffer, resolved);
#else

	if (_fullpath(buffer, path, max_len) == NULL)
		return -1;
#endif

	/* Replace all occurrences of Windows-style backslash with forward slash */
	while ((tmp = strchr(buffer, '\\')) != NULL)
		*tmp = '/';

	return 0;
}

size_t ws_exists(const char *const element, const char array[][MAX_ARG_SIZE], const size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		if (strcmp(element, array[i]) == 0)
		{
			return i;
		}
	}

	return SIZE_MAX;
}

int ws_is_dir_flag(const char *const flag)
{
	return flag[0] == '-' && flag[1] == 'I';
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
	workspace ws;
	ws_init(&ws);

	if (argv == NULL)
	{
		ws_add_error(&ws);
		return ws;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i] == NULL)
		{
			ws_add_error(&ws);
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
				ws_add_error(&ws);
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


workspace ws_create()
{
	workspace ws;
	ws_init(&ws);
	return ws;
}


size_t ws_add_file(workspace *const ws, const char *const path)
{
	if (!ws_is_correct(ws) || path == NULL)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	if (ws_unix_path(path, ws->files[ws->files_num],
	                 sizeof(ws->files[ws->files_num])) == -1 ||
	    access(ws->files[ws->files_num], F_OK) == -1)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	const size_t index = ws_exists(ws->files[ws->files_num], ws->files, ws->files_num);
	if (index != SIZE_MAX)
	{
		return index;
	}

	ws->files_num++;
	return ws->files_num - 1;
}

int ws_add_files(workspace *const ws, const char *const *const paths, const size_t num)
{
	if (paths == NULL)
	{
		ws_add_error(ws);
		return -1;
	}

	for (size_t i = 0; i < num; i++)
	{
		if (ws_add_file(ws, paths[i]) == SIZE_MAX)
		{
			return -1;
		}
	}
	return 0;
}


size_t ws_add_dir(workspace *const ws, const char *const path)
{
	if (!ws_is_correct(ws) || path == NULL)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	if (ws_unix_path(path, ws->dirs[ws->dirs_num],
	                 sizeof(ws->dirs[ws->dirs_num])) == -1 ||
	    access(ws->dirs[ws->dirs_num], F_OK) == -1)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	const size_t index = ws_exists(ws->dirs[ws->dirs_num], ws->dirs, ws->dirs_num);
	if (index != SIZE_MAX)
	{
		return index;
	}

	ws->dirs_num++;
	return ws->dirs_num - 1;
}

int ws_add_dirs(workspace *const ws, const char *const *const paths, const size_t num)
{
	if (paths == NULL)
	{
		ws_add_error(ws);
		return -1;
	}

	for (size_t i = 0; i < num; i++)
	{
		if (ws_add_dir(ws, paths[i]) == SIZE_MAX)
		{
			return -1;
		}
	}
	return 0;
}


size_t ws_add_flag(workspace *const ws, const char *const flag)
{
	if (!ws_is_correct(ws) || flag == NULL)
	{
		ws_add_error(ws);
		return SIZE_MAX;
	}

	if (ws_is_dir_flag(flag))
	{
		return ws_add_dir(ws, &flag[2]);
	}

	const size_t index = ws_exists(flag, ws->flags, ws->flags_num);
	if (index != SIZE_MAX)
	{
		return index;
	}

	strcpy(ws->flags[ws->flags_num++], flag);
	return ws->flags_num - 1;
}

int ws_add_flags(workspace *const ws, const char *const *const flags, const size_t num)
{
	if (flags == NULL)
	{
		ws_add_error(ws);
		return -1;
	}

	for (size_t i = 0; i < num; i++)
	{
		if (ws_add_flag(ws, flags[i]) == SIZE_MAX)
		{
			return -1;
		}
	}
	return 0;
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


int ws_is_correct(const workspace *const ws)
{
	return ws != NULL && !ws->was_error;
}


const char *ws_get_file(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) && index < ws->files_num ? ws->files[index] : NULL;
}

size_t ws_get_files_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws->files_num : 0;
}

const char *ws_get_dir(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) && index < ws->dirs_num ? ws->dirs[index] : NULL;
}

size_t ws_get_dirs_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws->dirs_num : 0;
}

const char *ws_get_flag(const workspace *const ws, const size_t index)
{
	return ws_is_correct(ws) && index < ws->flags_num ? ws->flags[index] : NULL;
}

size_t ws_get_flags_num(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws->flags_num : 0;
}


const char *ws_get_output(const workspace *const ws)
{
	return ws_is_correct(ws) ? ws->output : NULL;
}


int ws_clear(workspace *const ws)
{
	if (ws == NULL)
	{
		return -1;
	}

	ws->files_num = 0;
	ws->dirs_num = 0;
	ws->flags_num = 0;

	strcpy(ws->output, DEFAULT_OUTPUT);
	ws->was_error = 0;

	return 0;
}
