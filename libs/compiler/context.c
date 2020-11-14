/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov
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

#include "global.h"
#include "logger.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


#define DEFAULT_OUTBUF_SIZE (1024)


void compiler_context_init(compiler_context *context)
{
	memset(context, 0, sizeof(compiler_context));
	scanner_init(&context->input_options);
	printer_init(&context->output_options);
	printer_init(&context->err_options);
	printer_init(&context->miscout_options);
	compiler_table_init(&context->reprtab);

	context->charnum = 0;
	context->charnum_before = 0;
	REPRTAB_LEN = 1;
	context->id = 2;
	context->md = 1;
	context->startmode = 1;
	context->sopnd = -1;
	context->curid = 2;
	context->lg = -1;
	context->displ = -3;
	context->maxdispl = 3;
	context->maxdisplg = 3;
	context->procd = 1;
	context->pc = 4;
	context->funcnum = 2;
	context->blockflag = 1;
	context->notrobot = 1;
	context->prdf = -1;
	context->leftansttype = -1;
	context->c_flag = -1;
	context->buf_flag = 0;
	context->error_flag = 0;
	context->new_line_flag = 0;
	context->line = 1;
	context->charnum = 0;
	context->charnum_before = 0;
	context->buf_cur = 0;
	context->temp_tc = 0;
}

void compiler_context_deinit(compiler_context *context)
{
	scanner_deinit(&context->input_options);
	printer_deinit(&context->output_options);
	printer_deinit(&context->err_options);
	printer_deinit(&context->miscout_options);
}

/** Is I/O an actual output? */
static bool io_type_is_output(ruc_io_type type)
{
	switch (type)
	{
		case IO_TYPE_INPUT:
			return false;
		default:
			return true;
	}
}

/** Get type-specific options */
static void *io_type2opts(compiler_context *context, ruc_io_type type)
{
	switch (type)
	{
		case IO_TYPE_INPUT:
			return &context->input_options;
		case IO_TYPE_OUTPUT:
			return &context->output_options;
		case IO_TYPE_ERROR:
			return &context->err_options;
		case IO_TYPE_MISC:
			return &context->miscout_options;
		default:
			return NULL;
	}
}

/** Get access mask for a specific IO type */
static const char *io_type2access_mask(ruc_io_type type)
{
	return io_type_is_output(type) ? "wt" : "r";
}

/** Open file with specific mask taking standard files into account */
static FILE *io_get_file(const char *ptr, const char *mask)
{
	if (strcmp(ptr, ":stderr") == 0)
	{
		return stderr;
	}
	else if (strcmp(ptr, ":stdout") == 0)
	{
		return stdout;
	}
	else if (strcmp(ptr, ":stdin") == 0)
	{
		return stdin;
	}
	return fopen(ptr, mask);
}

void compiler_context_attach_io(compiler_context *context, const char *ptr, ruc_io_type type, ruc_io_source source)
{
	void *opts = io_type2opts(context, type);

	if (source == IO_SOURCE_FILE)
	{
		FILE *f = io_get_file(ptr, io_type2access_mask(type));

		if (f == NULL)
		{
			if (io_type_is_output(type))
			{
				char msg[4 * MAXSTRINGL];
				sprintf(msg, "%s: %s", ptr, strerror(errno));
				log_system_error("ruc", msg);
			}
			else
			{
				char msg[4 * MAXSTRINGL];
				sprintf(msg, "%s: No such file or directory\n", ptr);
				log_system_error("ruc", msg);
			}
			exit(1);
		}

		if (io_type_is_output(type))
		{
			printer_attach_file(opts, f);
		}
		else
		{
			scanner_attach_file(opts, f);
		}
	}
	else
	{
		if (io_type_is_output(type))
		{
			printer_attach_buffer(opts, DEFAULT_OUTBUF_SIZE);
		}
		else
		{
			scanner_attach_buffer(opts, ptr);
		}
	}
}

void compiler_context_detach_io(compiler_context *context, ruc_io_type type)
{
	void *opts = io_type2opts(context, type);

	if (io_type_is_output(type))
	{
		printer_close(opts);
	}
	else
	{
		scanner_close(opts);
	}
}

void compiler_table_init(compiler_table *table)
{
	table->table = malloc(COMPILER_TABLE_SIZE_DEFAULT * sizeof(int));
	if (table->table == NULL)
	{
		exit(1); // need a better way to stop!
	}
	table->pos = 0;
	table->len = 0;
	table->size = COMPILER_TABLE_SIZE_DEFAULT;
	memset(table->table, 0, COMPILER_TABLE_SIZE_DEFAULT * sizeof(int));
}

int compiler_table_ensure_allocated(compiler_table *table, int pos)
{
	if (unlikely(pos >= table->size))
	{
		int size = (table->size * 2 > (pos + COMPILER_TABLE_INCREMENT_MIN)) ? (table->size * 2)
																			: (pos + COMPILER_TABLE_INCREMENT_MIN);
		int *buf = realloc(table->table, sizeof(int) * size);

		if (buf == NULL)
		{
			exit(1);
		}

		memset(&buf[table->size], 0, size - table->size);
		table->table = buf;
		table->size = size;
	}

	return table->size;
}

int compiler_table_expand(compiler_table *table, int len)
{
	return compiler_table_ensure_allocated(table, table->len + len);
}