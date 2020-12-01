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
#include "preprocessor_utils.h"
#include "constants.h"
#include "commenter.h"
#include "file.h"
#include "preprocessor_error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Определение глобальных переменных
void con_files_init(files *fs)
{
	fs->p_s = 0;
	fs->p = MAX_ARG_SIZE + 1;

	fs->main_faile = -1;
	fs->cur = -1;

	fs->begin_f = MAX_ARG_SIZE + 1;
}

void con_init(preprocess_context *context)
{
	//printer_init(&context->output_options);
	context->io = io_create();

	con_files_init(&context->fs);

	context->include_type = 0;
	context->repr.p = 1;
	context->mp = 1;
	context->strp = 0;
	
	context->cp = 0;
	context->lsp = 0;
	context->csp = 0;
	context->ifsp = 0;
	context->wsp;
	context->prep_flag = 0;
	context->nextch_type = FILETYPE;
	context->nextp = 0;
	context->dipp = 0;
	context->line = 1;
	context->temp_output = 0;
	context->iwp = 0;
	context->h_flag = 0;
	context->current_p = 0;
	context->checkif = 0;
}

int con_repr_find_loc(reprtab* repr, char32_t* s, int hash)
{	
	int h = hash;
	if(h == 0)
	{
		int l = strlen32(s);
		for(int i = 0; i < l; i++)
		{
			h += s[i];
		}
		h &= HASH_M;
	}

	int r = repr->hashtab[h];
	while (r != 0)
	{
		int i = r + 2;
		int j = 0;

		while (repr->tab[i++] == s[j++])
		{
			if (repr->tab[i] == 0 && s[j] == 0 || s[j] == '\0')
			{
				return r+1;
			}
		}

		r = repr->tab[r];
	}
	return 0;
}


int con_repr_corect(reprtab *repr, int cod, int oldp, int hash, int find_flag)
{
	if(find_flag != 0)
	{
		int res = con_repr_find_loc(repr, &repr->tab[oldp+2], hash);
		if(res != 0)
		{
			repr->p = oldp;
			return res;
		}
	}
	repr->tab[repr->p++] = 0;
	repr->tab[oldp] = repr->hashtab[hash];
	repr->tab[oldp + 1] = cod;
	repr->hashtab[hash] = oldp;
	return 0;
}

int con_repr_add(reprtab *repr, char* s, int cod)
{	
	int oldp = repr->p;
	repr->p +=2;
	int hash = 0;

	int l = strlen(s);
	for(int i = 0; i < l; i++)
	{
		hash += s[i];
		repr->tab[repr->p++] = s[i];
	}
	hash &= HASH_M;

	if(cod == SH_FILE)
	{
		if(con_repr_corect(repr, cod, oldp, hash, 1) != 0)
		{
			return 0;
		}
	}
	else
	{
		con_repr_corect(repr, cod, oldp, hash, 0);
	}
	
	return 1;
}

void con_repr_add_ident(reprtab *repr, preprocess_context *context)
{	
	int oldp = repr->p;
	repr->p +=2;
	int hash = 0;

	while (is_letter(context) != 0 || is_digit(context->curchar)!= 0)
	{
		hash += context->curchar;
		repr->tab[repr->p++] = context->curchar;
		m_nextch(context);
	}

	hash &= HASH_M;
	int res =  con_repr_corect(repr, context->mp, oldp, hash, 1);
	if(res == 0)
	{
		if(context->macrotext[repr->tab[res]] == MACROUNDEF)
		{
			repr->tab[res] = context->mp;
		}
		//else error
	}
}

int con_repr_find(reprtab* repr, char32_t* s)
{
	int res = con_repr_find_loc(repr, &s[1], s[0]);
	if(res == 0)
	{
		return 0;
	}
	else
	{
		return repr->tab[res];
	}
}

void con_repr_change(reprtab *repr, preprocess_context *context)
{
	int oldp = repr->p;
	repr->p +=2;
	int hash = 0;

	while (is_letter(context) != 0|| is_digit(context->curchar) != 0)
	{
		hash += context->curchar;
		repr->tab[repr->p++] = context->curchar;
		m_nextch(context);
	}

	hash &= HASH_M;
	int res = con_repr_corect(repr, context->mp, oldp, hash, 1);

	if(res == 0)
	{
		repr->p = oldp;//error
	}

	if(context->macrotext[repr->tab[res]] == MACROFUNCTION || 
	context->macrotext[repr->tab[res]] ==MACROUNDEF)
	{
		m_error(functions_cannot_be_changed, context);
	}

	repr->tab[res] = context->mp;
}




void con_file_add(file *f, const char *name, int cnost_name)
{
	f->const_name = cnost_name;
	if(cnost_name != 0)
	{
		f->name = name;
	}
	else
	{
		f->name = malloc((strlen(name) + 1) * sizeof(char));
		strcpy(f->name, name);
	}
	
}

void con_file_free(file *f )
{
	if (f->const_name == 0)
	{
		free(f->name);
	}
}


void con_files_add_parametrs(files* fs, const char *name)
{
	con_file_add(&fs->files[fs->p_s++], name, 1);
}

void con_files_add_include(files* fs, const char *name)
{
	fs->cur = fs->p;
	con_file_add(&fs->files[fs->p++], name, 0);
}

void con_files_free(files *fs)
{
	for (int i = fs->begin_f; i < fs->p; i++)
	{
		if(&fs->files[i] != NULL && fs->files[i].name != NULL)
		{
			con_file_free(&fs->files[i]);
		}
	}
}

void con_file_open_cur(files* fs, preprocess_context *context)
{
	context->current_file = fopen(fs->files[fs->cur].name, "r");

	if (context->current_file == NULL)
	{
		log_system_error(fs->files[fs->cur].name, "файл не найден");
		m_error(just_kill_yourself, context);
	}
}

int con_file_open_main(files* fs, preprocess_context *context)
{	
	
	if(fs->main_faile == -1)
	{
		return 0;
	}

	fs->cur = fs->main_faile;

	con_file_open_cur(&context->fs, context);

	return 1;
}

int con_file_open_sorse(files* fs, preprocess_context *context)
{
	fs->cur = 0;
	if(fs->cur == fs->main_faile)
	{
		fs->cur++;
	}

	if(fs->cur == fs->p_s)
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
	if((h_flag != 0 && (fs->cur >= fs->begin_f && fs->cur < fs->end_h )) || 
		(h_flag == 0 && fs->cur < fs->begin_f && fs->cur < fs->p_s - 1))
	{
		fs->cur++;
		if(h_flag == 0 && fs->cur == fs->main_faile)
		{
			fs->cur++;
		}
	}
	else
	{
		return 0;
	}
	
	if((h_flag  != 0 && fs->cur == fs->end_h) || (h_flag == 0 && fs->cur == fs->p_s))
	{
		return 0;
	}

	con_file_open_cur(&context->fs, context);

	return 1;
}

void con_file_it_is_main(files *fs)
{
	fs->main_faile = fs->cur;
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
	free(buf);
}

void con_switch_io_add(switch_io switcher, int type_io, char c) ;
void con_switch_io_clean(switch_io switcher, int type_io);
void con_switch_io_cange(switch_io switcher, int type_io, size_t p);
void con_switch_io_back(switch_io switcher);
void con_switch_io_nextch(switch_io switcher);
//con_switch_get_curchar(switch_io switcher): char
//con_switch_get_nextchar(switch_io switcher): char

