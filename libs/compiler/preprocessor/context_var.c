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
#include "file.h"
#include "preprocessor_error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Определение глобальных переменных
void preprocess_context_init(preprocess_context *context, int num)
{
	printer_init(&context->output_options);

	con_files_init(&context->fs, num);

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
	context->wsp;
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


void con_file_add(file *f, const char *name, int cnost_name)
{
	f->name = name;
	f->const_name = cnost_name;
}

void con_file_free(file *f )
{
	if (!f->const_name)
	{
		free(f->name);
	}
}

void con_files_init(files *fs, int num)
{
	fs->size = num*3;
	fs->files = malloc(fs->size * sizeof(file));

	fs->p_s = 0;
	fs->p = num + 1;

	fs->main_faile = -1;
	fs->cur = -1;

	fs->begin_h = num + 1;
}

void con_files_add_parametrs(files* fs, const char *name)
{
	con_file_add(&fs->files[fs->p_s++], name, 1);
}

void con_files_add_include(files* fs, const char *name)
{
	/*if (s->p == s->size)
	{
		s->size *= 2;
		data_file *reallocated = realloc(s->files, s->size * sizeof(data_file));
		s->files = reallocated;
	}*/

	fs->cur = fs->p;
	con_file_add(&fs->files[fs->p++], name, 0);
}

void con_files_free(files *fs)
{
	for (int i = 0; i < fs->p; i++)
	{
		con_file_free(&fs->files[i]);
	}

	free(fs->files);
}

void con_file_open_main(files* fs, preprocess_context *context)
{
   fs->cur = fs->main_faile;
   context->current_file = fopen(fs->files[fs->cur].name, "r");
   	if (context->current_file == NULL)
	{
		log_system_error(fs->files[fs->cur].name, "файл не найден");
		m_error(just_kill_yourself, context);
	}
}

int con_file_open(files* fs, preprocess_context *context, int h_flag)
{
	if((h_flag && (fs->cur >= fs->begin_h && fs->cur < fs->end_h )) || 
		!h_flag && fs->cur < fs->begin_h)
	{
		fs->cur++;
		if(!h_flag && fs->cur == fs->main_faile)
		{
			fs->cur++;
		}
	}
	else if(fs->end_h <= fs->begin_h)
	{
		return 1;
	}
	else if(!h_flag)
	{
		fs->cur = 0;
	}
	else
	{
		fs->cur = fs->begin_h;
	}

	if((h_flag && fs->cur == fs->end_h) || (!h_flag && fs->cur == fs->p_s))
	{
		return 1;
	}
	
	
	context->current_file = fopen(fs->files[fs->cur].name, "r");

	if (context->current_file == NULL)
	{
		log_system_error(fs->files[fs->cur].name, "файл не найден");
		m_error(just_kill_yourself, context);
	}

	return 0;
	

}

void con_file_it_is_main(files *fs)
{
	fs->main_faile = fs->p_s - 1;
}

void con_file_it_is_end_h(files *fs)
{
	fs->end_h = fs->p;
}

void con_file_close_cur(preprocess_context *context)
{
	fclose(context->current_file);
	context->current_file = NULL;
	context->line = 1;
}

void con_file_print_coment(files *fs, preprocess_context *context)
{
	comment com = cmt_create(fs->files[fs->cur].name, context->line-1);
	char *buf = malloc(100 * sizeof(char *));
	size_t size = cmt_to_string(&com, buf);
	for(size_t i = 0; i < size; i++)
	{
		m_fprintf(buf[i], context);
	}
}

