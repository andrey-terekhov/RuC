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
#include "macro_global_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_SIZE 10000

void macro_long_string_init(macro_long_string *s)
{
	s->size = DEFAULT_SIZE;
	s->p = 0;
	s->str = NULL;
}


void data_files_init(data_files *s)
{
	s->size = 10;
	s->p = 0;
	s->cur = -1;
	s->files = malloc(s->size * sizeof(data_file));
}

// Определение глобальных переменных
void preprocess_context_init(preprocess_context *context)
{
	memset(context, 0, sizeof(preprocess_context));
	printer_init(&context->output_options);
	data_files_init(&context->c_files);
	data_files_init(&context->h_files);
	macro_long_string_init(&context->befor_temp);
	context->include_type = 0;
	context->rp = 1;
	context->inp_file = 0;
	context->inp_p = -1;
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
	context->nextch_type = 0;
	context->nextp = 0;
	context->main_file = -1;
	context->dipp = 0;
	context->line = 1;
	context->temp_output = 0;
	context->control_aflag = 0;
	context->control_bflag = 0;
	context->befor_temp_p = -1;
	context->iwp = 0;
}