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

#pragma once

#include "constants.h"
#include "macro_global_struct.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct preprocess_context
{
	int include_type;

	int hashtab[256];
	int reprtab[MAXTAB];
	int rp;

	int macrotext[MAXTAB];
	int mp;
	int oldmp;

	int mstring[STRIGSIZE];
	int msp;

	int strp;

	int fchange[STRIGSIZE * 3];
	int cp;

	int localstack[STRIGSIZE];
	int lsp;

	int cstring[STRIGSIZE];
	int csp;

	int ifstring[STRIGSIZE * 2];
	int ifsp;

	int wstring[STRIGSIZE * 5];
	int wsp;

	int mfirstrp;

	int mclp;

	int prep_flag;

	int curchar, nextchar;
	int nextch_type;
	int cur;

	int nextp;
	int main_file;

	int oldcurchar[DIP];
	int oldnextchar[DIP];
	int oldnextch_type[DIP];
	int oldnextp[DIP];
	int dipp;

	int line;
	int control_aflag;
	int control_bflag;

	int temp_output;
	data_files *sources;
	data_files *headers;
	int h_flag;

	int FILE_flag;
	FILE *current_file;
	macro_long_string *befor_temp;
	int befor_temp_p;
	int *current_string;
	int current_p;

	const char **include_ways;
	int iwp;

	universal_printer_options output_options;
} preprocess_context;

void preprocess_context_init(preprocess_context *context, data_files *sources, data_files *headers);

#ifdef __cplusplus
} /* extern "C" */
#endif
