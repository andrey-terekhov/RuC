#include "parser.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


const char *const default_output = "export.txt";


workspace ws_parse_args(const int argc, const char *const *const argv)
{
	workspace ws;
	return ws;
}


workspace ws_create()
{
	workspace ws;
	return ws;
}


int ws_add_file(workspace *const ws, const char *const path)
{
	return 0;
}

int ws_add_files(workspace *const ws, const char *const *const paths)
{
	return 0;
}


int ws_add_dir(workspace *const ws, const char *const path)
{
	return 0;
}

int ws_add_dirs(workspace *const ws, const char *const *const paths)
{
	return 0;
}


int ws_add_flag(workspace *const ws, const char *const flag)
{
	return 0;
}

int ws_add_flags(workspace *const ws, const char *const *const flags)
{
	return 0;
}


int ws_set_output(workspace *const ws, const char *const path)
{
	return 0;
}


int ws_is_correct(const workspace *const ws)
{
	return 0;
}


const char *const *ws_get_file_list(const workspace *const ws)
{
	return NULL;
}

const char *const *ws_get_dir_list(const workspace *const ws)
{
	return NULL;
}

const char *const *ws_get_flag_list(const workspace *const ws)
{
	return NULL;
}


const char *ws_get_output(const workspace *const ws)
{
	return NULL;
}


int ws_clear(workspace *const ws)
{
	return 0;
}
