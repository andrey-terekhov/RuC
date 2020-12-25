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

#include "context_var.h"
#include "constants.h"
#include "commenter.h"
#include "workspace.h"
#include "file.h"
#include "error.h"
#include "logger.h"
#include "uniprinter.h"
#include "uniio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMT_SIZE MAX_ARG_SIZE + 32

void con_files_init(files *fs, workspace *const ws)
{
	fs->ws = ws;
	fs->cur = MAX_PATHS;
	size_t end_sorse = 0;
	for (size_t i = 0; i < MAX_PATHS; i++)
	{
		fs->already_included[i] = 0;
	}
}

void preprocess_context_init(preprocess_context *context, workspace *const ws, universal_io *const io, universal_io *const io_input)
{
	context->io_output = io;
	context->io_input = io_input;

	con_files_init(&context->fs, ws);

	context->rp = 1;
	context->mp = 1;
	context->msp = 0;
	context->cp = 0;
	context->lsp = 0;
	context->csp = 0;
	context->ifsp = 0;
	context->wsp = 0;
	context->mfirstrp = -1;
	context->prep_flag = 0;
	context->nextch_type = FILETYPE;
	context->curchar = 0;
	context->nextchar = 0;
	context->cur = 0;
	context->nextp = 0;
	context->dipp = 0;
	context->line = 1;
	context->h_flag = 0;
	context->position = 0;

	for (int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
	}

	for (int i = 0; i < MAXTAB; i++)
	{
		context->reprtab[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		context->mstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		context->error_string[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE*3; i++)
	{
		context->fchange[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		context->localstack[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		context->cstring[i] = 0;
	}
	
	for (int i = 0; i < STRING_SIZE*2; i++)
	{
		context->ifstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE*5; i++)
	{
		context->wstring[i] = 0;
	}

	for (int i = 0; i < DIP; i++)
	{
		context->oldcurchar[i] = 0;
		context->oldnextchar[i] = 0;
		context->oldnextch_type[i] = 0;
		context->oldnextp[i] = 0;
	}
}

int con_file_open(files* fs, preprocess_context *context, size_t indext)
{
	if (in_set_file(context->io_input, ws_get_file(fs->ws, indext)))
	{
		log_system_error(ws_get_file(fs->ws, fs->cur), "файл не найден");
		return -1;
	}
	return 0;
}

void con_file_print_coment(files *fs, preprocess_context *context)
{
	comment cmt = cmt_create(ws_get_file(fs->ws, fs->cur), context->line);

	char buffer[MAX_CMT_SIZE];
	cmt_to_string(&cmt, buffer);

	uni_printf(context->io_output, "%s", buffer);
}
