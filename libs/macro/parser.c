/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
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

#include "parser.h"
#include "uniprinter.h"
#include "uniscanner.h"

parser parser_create(linker *const lk, /*storage *const stg,*/ universal_io *const out)
{
	parser prs; 
	if(!linker_is_correct(lk) || !out_is_correct(out))// || !storage_is_correct(stg))
	{
		prs.lk = NULL;
		return prs;
	}

	prs.lk = lk;
	//prs.stg = stg;
	prs.out = out;

	prs.in = NULL;
	prs.error_line = 0;
	prs.is_recovery_disabled = false;

	return prs;
} 


int parser_preprocess(parser *const prs, universal_io *const in)
{
	if (!parser_is_correct(prs)|| in_is_correct(in))
	{
		return -1;
	}

	prs->in = in;
	char32_t cur = uni_scan_char(prs->in);// хранить в парсере ?
	while (cur != EOF)
	{
		uni_print_char(prs->out, cur);
		char32_t cur = uni_scan_char(prs->in);
	}
	return 0;
}


int parser_disable_recovery(parser *const prs)
{
	if (!parser_is_correct(prs))
	{
		return -1;
	}

	prs->is_recovery_disabled = true;
}


bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && in_is_correct(prs->in)
		&& out_is_correct(prs->out);// && storage_is_correct(prs->stg);
}