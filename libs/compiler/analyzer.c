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
#include "keywords.h"
#include "lexer.h"
#include "uniio.h"
#include "string.h"


parser compiler_context_create(universal_io *const io, syntax *const sx, lexer *const lexer)
{
	parser context;
	context.io = io;
	context.sx = sx;
	context.lexer = lexer;

	context.sp = 0;
	context.sopnd = -1;
	context.leftansttype = -1;
	context.buf_flag = 0;
	context.was_error = 0;
	context.buf_cur = 0;
	context.labels = vector_create(100);
	
	context.anstdispl = 0;

	return context;
}


/** Занесение ключевых слов в reprtab */
void read_keywords(parser *context)
{
	context->sx->keywords = 1;
	get_char(context->lexer);
	get_char(context->lexer);
	while (lex(context->lexer) != LEOF)
	{
		; // чтение ключевых слов
	}
}


size_t toreprtab(parser *context, char str[])
{
	int i;
	size_t oldrepr = REPRTAB_LEN;

	context->sx->hash = 0;

	REPRTAB_LEN += 2;
	for (i = 0; str[i] != 0; i++)
	{
		context->sx->hash += str[i];
		REPRTAB[REPRTAB_LEN++] = str[i];
	}
	context->sx->hash &= 255;

	REPRTAB[REPRTAB_LEN++] = 0;

	REPRTAB[oldrepr] = (int)context->sx->hashtab[context->sx->hash];
	REPRTAB[oldrepr + 1] = 1;
	return context->sx->hashtab[context->sx->hash] = oldrepr;
}

/** Инициализация modetab */
void init_modetab(parser *context)
{
	// занесение в modetab описателя struct {int numTh; int inf; }
	vector_add(&context->sx->modes, 0);
	vector_add(&context->sx->modes, mode_struct);
	vector_add(&context->sx->modes, 2);
	vector_add(&context->sx->modes, 4);
	vector_add(&context->sx->modes, mode_integer);
	vector_add(&context->sx->modes, (item_t)toreprtab(context, "numTh"));
	vector_add(&context->sx->modes, mode_integer);
	vector_add(&context->sx->modes, (item_t)toreprtab(context, "data"));

	// занесение в modetab описателя функции void t_msg_send(struct msg_info m)
	vector_add(&context->sx->modes, 1);
	vector_add(&context->sx->modes, mode_function);
	vector_add(&context->sx->modes, mode_void);
	vector_add(&context->sx->modes, 1);
	vector_add(&context->sx->modes, 2);

	// занесение в modetab описателя функции void* interpreter(void* n)
	vector_add(&context->sx->modes, 9);
	vector_add(&context->sx->modes, mode_function);
	vector_add(&context->sx->modes, mode_void_pointer);
	vector_add(&context->sx->modes, 1);
	vector_add(&context->sx->modes, mode_void_pointer);

	context->sx->start_mode = 14;
	context->sx->keywords = 0;
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
	lexer lexer = create_lexer(&temp, sx);
	parser context = compiler_context_create(&temp, sx, &lexer);
	
	in_set_buffer(context.io, KEYWORDS);
	read_keywords(&context);
	in_clear(context.io);

	init_modetab(&context);

	io_erase(&temp);

	context.io = io;
	context.lexer->io = io;
	return parse(&context);
}
