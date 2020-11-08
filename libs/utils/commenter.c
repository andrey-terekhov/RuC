#include "commenter.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

#define CMT_INCORRECT 0
//#define CMT_ 1
#define CMT_MACRO 2

/*struct comment 
{
	char path[100MAX_STRING];
	size_t line;
	size_t symbol;
	int type;
};*/

comment cmt_create(const char *const path, const size_t line)	//Надо делать, алгоритм проверки на корректность не придумал
{
	comment cmt;
	strcpy(cmt.path, path);
	cmt.line = line;
	cmt.type = 1;
	return cmt;
}

comment cmt_create_macro(const char *const path, const size_t line, const size_t symbol)
{
	comment cmt;
	return cmt;
}


int cmt_to_buffer(const comment *const cmt, char *const buffer)
{
	return 0;
}

comment cmt_search(const char *const code, const size_t position)
{
	comment cmt;
	return cmt;
}


int cmt_is_correct(const comment *const cmt)
{
	return cmt->type != CMT_INCORRECT ? 1 : 0;
}

int cmt_is_macro(const comment *const cmt)
{
	return cmt->type == CMT_MACRO ? 1 : 0;
}


const char *cmt_get_path(const comment *const cmt)
{
	return cmt->path;
}

size_t cmt_get_line(const comment *const cmt)
{
	return cmt->line;
}

size_t cmt_get_symbol(const comment *const cmt)
{
	return cmt->symbol;
}
