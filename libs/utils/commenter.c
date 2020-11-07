#include "commenter.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

int comment(char *const comment, const char *const path, const size_t line)
{
	return 0;
}

int macro_comment(char *const comment, const char *const path, const size_t line, const size_t symbol)
{
	return 0;
}

const char * search_comment(const char *const code, const size_t position)
{
	return '1';
}

int extract_comment(const char *const comment, char *const path, size_t *const line, size_t *const symbol)
{
	return 0;
}
