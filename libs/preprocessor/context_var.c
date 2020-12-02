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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Определение глобальных переменных
void con_files_init(files *fs, workspace *const ws)
{
	fs->p = 0;
	fs->cur = -1;
	fs->end_h = 0;
	fs->begin_f = -1;
	fs->ws = ws;
}

void preprocess_context_init(preprocess_context *context, workspace *const ws, universal_io *const io)
{
	context->io = io;

	con_files_init(&context->fs, ws);

	context->include_type = 0;
	context->rp = 1;
	context->mp = 1;
	context->strp = 0;
	context->oldmp = 1;
	context->msp = 0;
	context->cp = 0;
	context->lsp = 0;
	context->csp = 0;
	context->ifsp = 0;
	context->wsp = 0;
	context->mfirstrp = -1;
	context->prep_flag = 0;
	context->mclp = 1;
	context->nextch_type = FILETYPE;
	context->nextp = 0;
	context->main_file = -1;
	context->dipp = 0;
	context->line = 1;
	context->temp_output = 0;
	context->iwp = 0;
	context->h_flag = 0;
	context->current_p = 0;

	for (int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
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
	context->current_file = fopen(ws_get_file(fs->ws, fs->cur), "r");

	if (context->current_file == NULL)
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
	fclose(context->current_file);
	context->current_file = NULL;
	context->line = 1;
}

void con_file_print_coment(files *fs, preprocess_context *context)
{
	comment com = cmt_create(ws_get_file(fs->ws, fs->cur), context->line-1);
	char *buf = malloc(100 * sizeof(char *));
	size_t size = cmt_to_string(&com, buf);
	for(size_t i = 0; i < size; i++)
	{
		m_fprintf(buf[i], context);
	}
}

