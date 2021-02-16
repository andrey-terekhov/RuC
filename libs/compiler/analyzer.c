/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov, Dmitrii Davladov
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

#include "analyzer.h"
#include "parser.h"
#include "codes.h"
#include "keywords.h"
#include "lexer.h"
#include "tree.h"


const char *const DEFAULT_TREE = "tree.txt";
const char *const DEFAULT_NEW = "new.txt";


parser parser_create(syntax *const sx, lexer *const lxr)
{
	parser prs;
	prs.sx = sx;
	prs.lxr = lxr;

	prs.sp = 0;
	prs.sopnd = -1;
	prs.leftansttype = -1;
	prs.buf_flag = 0;
	prs.was_error = 0;
	prs.buf_cur = 0;

	prs.anstdispl = 0;

	return prs;
}


/** Занесение ключевых слов в reprtab */
void read_keywords(parser *const prs)
{
	prs->sx->keywords = 1;
	get_char(prs->lxr);
	get_char(prs->lxr);
	while (lex(prs->lxr) != LEOF)
	{
		; // чтение ключевых слов
	}
	prs->sx->keywords = 0;
}


size_t toreprtab(parser *const prs, char str[])
{
	size_t oldrepr = REPRTAB_LEN;

	prs->sx->hash = 0;

	REPRTAB_LEN += 2;
	for (int i = 0; str[i] != '\0'; i++)
	{
		prs->sx->hash += str[i];
		REPRTAB[REPRTAB_LEN++] = str[i];
	}
	prs->sx->hash &= 255;

	REPRTAB[REPRTAB_LEN++] = 0;

	REPRTAB[oldrepr] = (int)prs->sx->hashtab[prs->sx->hash];
	REPRTAB[oldrepr + 1] = 1;
	return prs->sx->hashtab[prs->sx->hash] = oldrepr;
}

/** Инициализация modetab */
void init_modetab(parser *const prs)
{
	// занесение в modetab описателя struct {int numTh; int inf; }
	vector_add(&prs->sx->modes, 0);
	vector_add(&prs->sx->modes, mode_struct);
	vector_add(&prs->sx->modes, 2);
	vector_add(&prs->sx->modes, 4);
	vector_add(&prs->sx->modes, mode_integer);
	vector_add(&prs->sx->modes, (item_t)toreprtab(prs, "numTh"));
	vector_add(&prs->sx->modes, mode_integer);
	vector_add(&prs->sx->modes, (item_t)toreprtab(prs, "data"));

	// занесение в modetab описателя функции void t_msg_send(struct msg_info m)
	vector_add(&prs->sx->modes, 1);
	vector_add(&prs->sx->modes, mode_function);
	vector_add(&prs->sx->modes, mode_void);
	vector_add(&prs->sx->modes, 1);
	vector_add(&prs->sx->modes, 2);

	// занесение в modetab описателя функции void* interpreter(void* n)
	vector_add(&prs->sx->modes, 9);
	vector_add(&prs->sx->modes, mode_function);
	vector_add(&prs->sx->modes, mode_void_pointer);
	vector_add(&prs->sx->modes, 1);
	vector_add(&prs->sx->modes, mode_void_pointer);

	prs->sx->start_mode = 14;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int analyze(universal_io *const io, syntax *const sx)
{
	if (!in_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	universal_io temp = io_create();
	lexer lxr = create_lexer(&temp, sx);
	parser prs = parser_create(sx, &lxr);

	in_set_buffer(prs.lxr->io, KEYWORDS);
	read_keywords(&prs);
	in_clear(prs.lxr->io);

	init_modetab(&prs);

	io_erase(&temp);

	prs.lxr->io = io;

#ifndef GENERATE_TREE
	return parse(&prs) || !sx_is_correct(sx);
#else
	const int ret = parse(&prs) || !sx_is_correct(sx)
		|| tree_test(&sx->tree)
		|| tree_test_next(&sx->tree)
		|| tree_test_recursive(&sx->tree)
		|| tree_test_copy(&sx->tree);

	tables_and_tree(DEFAULT_TREE, &sx->identifiers, &sx->modes, &sx->tree);

	if (!ret)
	{
		tree_print(DEFAULT_NEW, &sx->tree);
	}
	return ret;
#endif
}
