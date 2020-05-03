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
#include <string.h>


// Определение глобальных переменных
void preprocess_context_init(preprocess_context *context)
{
	memset(context, 0, sizeof(preprocess_context));
	context->rp = 1;
	context->inp_file = 0;
	context->inp_p = 0;
	context->rp = 1;
	context->mp = 1;
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
}
