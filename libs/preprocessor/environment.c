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

#include "environment.h"


void env_init(environment *const env, linker *const lk, universal_io *const output)
{
	env->output = output;

	env->lk = lk;

	env->rp = 1;
	env->mp = 1;
	env->cp = 0;
	env->lsp = 0;
	env->csp = 0;
	env->ifsp = 0;
	env->wsp = 0;
	env->mfirstrp = -1;
	env->prep_flag = 0;
	env->nextch_type = FILETYPE;
	env->curchar = 0;
	env->nextchar = 0;
	env->cur = 0;
	env->nextp = 0;
	env->dipp = 0;
	env->line = 1;
	env->position = 0;

	for (int i = 0; i < HASH; i++)
	{
		env->hashtab[i] = 0;
	}

	for (int i = 0; i < MAXTAB; i++)
	{
		env->reprtab[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		env->mstring[i] = 0;
		env->error_string[i] = 0;
		env->localstack[i] = 0;
		env->cstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE * 3; i++)
	{
		env->fchange[i] = 0;
	}

	
	for (int i = 0; i < STRING_SIZE * 2; i++)
	{
		env->ifstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE * 5; i++)
	{
		env->wstring[i] = 0;
	}

	for (int i = 0; i < DIP; i++)
	{
		env->oldcurchar[i] = 0;
		env->oldnextchar[i] = 0;
		env->oldnextch_type[i] = 0;
		env->oldnextp[i] = 0;
	}
}

void env_clear_error_string(environment *const env)
{
	env->position = 0;
}

