/*
 *	Copyright 2019 Andrey Terekhov
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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int toreprtab(compiler_context *context, char str[])
{
	int i;
	int oldrepr = REPRTAB_LEN;
	context->hash = 0;
	compiler_table_expand(&context->reprtab, 2);
	REPRTAB_LEN += 2;
	for (i = 0; str[i] != 0; i++)
	{
		compiler_table_expand(&context->reprtab, 1);
		context->hash += str[i];
		REPRTAB[REPRTAB_LEN++] = str[i];
	}
	context->hash &= 255;
	compiler_table_expand(&context->reprtab, 1);
	REPRTAB[REPRTAB_LEN++] = 0;
	compiler_table_ensure_allocated(&context->reprtab, oldrepr + 1);
	REPRTAB[oldrepr] = context->hashtab[context->hash];
	REPRTAB[oldrepr + 1] = 1;
	return context->hashtab[context->hash] = oldrepr;
}

/* Инициализация modetab */
void init_modetab(compiler_context *context)
{
	// занесение в modetab описателя struct {int numTh; int inf; }
	context->modetab[1] = 0;
	context->modetab[2] = MSTRUCT;
	context->modetab[3] = 2;
	context->modetab[4] = 4;
	context->modetab[5] = context->modetab[7] = LINT;
	context->modetab[6] = toreprtab(context, "numTh");
	context->modetab[8] = toreprtab(context, "data");

	// занесение в modetab описателя функции void t_msg_send(struct msg_info m)
	context->modetab[9] = 1;
	context->modetab[10] = MFUNCTION;
	context->modetab[11] = LVOID;
	context->modetab[12] = 1;
	context->modetab[13] = 2;

	// занесение в modetab описателя функции void* interpreter(void* n)
	context->modetab[14] = 9;
	context->modetab[15] = MFUNCTION;
	context->modetab[16] = LVOIDASTER;
	context->modetab[17] = 1;
	context->modetab[18] = LVOIDASTER;
	context->modetab[19] = context->startmode = 14;
	context->md = 19;
	context->keywordsnum = 0;
	context->lines[context->line = 1] = 1;
	context->charnum = 1;
	context->kw = 1;
	context->tc = 0;
}
