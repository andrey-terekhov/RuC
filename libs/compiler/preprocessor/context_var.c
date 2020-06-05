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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_SIZE 10000

macro_long_string_init(macro_long_string *s)
{
	s->size = DEFAULT_SIZE;
	s->p = 0;
	s->str = malloc(DEFAULT_SIZE * sizeof(int));
	memset(s->str, 0, DEFAULT_SIZE * sizeof(int));
}

control_string_init(control_string *s)
{
	s->size = DEFAULT_SIZE;
	s->p = 0;
	s->str_before = malloc(DEFAULT_SIZE * sizeof(int));
	s->str_after = malloc(DEFAULT_SIZE * sizeof(int));
	memset(s->str_before, 0, DEFAULT_SIZE * sizeof(int));
	memset(s->str_after, 0, DEFAULT_SIZE * sizeof(int));
}


// Определение глобальных переменных
void preprocess_context_init(preprocess_context *context)
{
	memset(context, 0, sizeof(preprocess_context));
	macro_long_string_init(&context->error_input);
	control_string_init(&context->control);
	printer_init(&context->output_options);
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
	context->mclp = 1;
	context->nextch_type = 0;
	context->nextp = 0;
	context->dipp = 0;
	context->line = 1;
	context->temp_output = 0;
	context->control_aflag = 0;
	context->control_bflag = 0;
}

void long_string_pinter(macro_long_string *s, int a)
{
	if(s->p == s->size - 1)
	{
		s->size *= 2;
		int *reallocated = realloc(s->str, s->size * sizeof(int));
		memset(&reallocated[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		s->str = reallocated;
	} 
	s->str[s->p++] = a;
}

void control_string_pinter(control_string *s, int before, int after)
{
	if(s->p == s->size - 1)
	{
		s->size *= 2;
		int *reallocated_b = realloc(s->str_before, s->size * sizeof(int));
		int *reallocated_a = realloc(s->str_after, s->size * sizeof(int));
		memset(&reallocated_b[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		memset(&reallocated_a[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		s->str_before = reallocated_b;
		s->str_after = reallocated_a;
	} 
	s->str_before[s->p] = before;
	s->str_after[s->p] = after;
	s->p++;
}