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
#include "preprocessor_error.h"
#include "logger.h"
#include "uniprinter.h"
#include "uniio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMT_SIZE MAX_ARG_SIZE + 32

// Определение глобальных переменных
void con_files_init(files *fs, workspace *const ws)
{
	fs->p = 0;
	fs->cur = -1;
	fs->end_h = 0;
	fs->begin_f = -1;
	fs->ws = ws;
}

void preprocess_context_init(preprocess_context *context, workspace *const ws, universal_io *const io, universal_io *const io_input)
{
	context->io_output = io;
	context->io_input = io_input;

	con_files_init(&context->fs, ws);

	context->include_type = 0;
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

	for (int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
	}

	for (int i = 0; i < MAXTAB; i++)
	{
		context->reprtab[i] = 0;
	}

	for (int i = 0; i < STRIGSIZE; i++)
	{
		context->mstring[i] = 0;
	}

	for (int i = 0; i < STRIGSIZE*3; i++)
	{
		context->fchange[i] = 0;
	}

	for (int i = 0; i < STRIGSIZE; i++)
	{
		context->localstack[i] = 0;
	}

	for (int i = 0; i < STRIGSIZE; i++)
	{
		context->cstring[i] = 0;
	}
	
	for (int i = 0; i < STRIGSIZE*2; i++)
	{
		context->ifstring[i] = 0;
	}

	for (int i = 0; i < STRIGSIZE*5; i++)
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

void con_files_add_include(files* fs, char *name, int c_flag)
{
	fs->p++;
	
	if(c_flag == 0)
	{
		fs->end_h++;
	}
	else
	{
		fs->cur = fs->p;
	}
	
	ws_add_file(fs->ws, name);
}

void con_file_open_cur(files* fs, preprocess_context *context)
{
	int rez = in_set_file(context->io_input, ws_get_file(fs->ws, fs->cur));

	if (rez == -1)
	{
		log_system_error(ws_get_file(fs->ws, fs->cur), "файл не найден");
		m_error(just_kill_yourself, context);
	}
}


int con_file_open_sorse(files* fs, preprocess_context *context)
{
	fs->cur = 0;

	if(fs->cur == fs->begin_f)
	{
		return 0;
	}

	con_file_open_cur(&context->fs, context);

	return 1;
}

int con_file_open_hedrs(files* fs, preprocess_context *context)
{
	if( fs->end_h > fs->begin_f)
	{
		fs->cur = fs->begin_f;
	}
	else
	{
		return 0;
	}

	con_file_open_cur(&context->fs, context);

	return 1;
}

int con_file_open_next(files* fs, preprocess_context *context, int h_flag)
{
	if((h_flag && (fs->cur >= fs->begin_f && fs->cur < fs->end_h - 1)) || 
		(!h_flag && (fs->begin_f < 0 || fs->cur < fs->begin_f - 1)))
	{
		fs->cur++;
	}
	else
	{
		return 0;
	}

	con_file_open_cur(&context->fs, context);

	return 1;
}

void con_file_it_is_end_h(files *fs, int i)
{
	fs->end_h += i;
	fs->p += i;
	fs->begin_f = i;
}

void con_file_close_cur(preprocess_context *context)
{
	in_clear(context->io_input);
	context->line = 1;
}

void con_file_print_coment(files *fs, preprocess_context *context)
{
	comment cmt = cmt_create(ws_get_file(fs->ws, fs->cur), context->line-1);

	char buffer[MAX_CMT_SIZE];
	cmt_to_string(&cmt, buffer);

	uni_printf(context->io_output, "%s", buffer);
}
