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

// Определение глобальных переменных
void preprocess_context_init(preprocess_context *context, data_files *sources, data_files *headers)
{
	printer_init(&context->output_options);
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
	context->control_aflag = 0;
	context->control_bflag = 0;
	context->before_temp_p = -1;
	context->iwp = 0;
	context->FILE_flag = 1;
	context->h_flag = 0;
	context->current_p = 0;

	context->sources = sources;
	context->headers = headers;

	context->before_temp = NULL;

	for (int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
	}
}