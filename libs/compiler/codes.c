/*
 *	Copyright 2014 Andrey Terekhov
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

#include "codes.h"
#include <string.h>
#include "errors.h"
#include "tree.h"
#include "operations.h"
#include "uniio.h"
#include "uniprinter.h"


#define MAX_ELEM_SIZE	32
#define INDENT			"  "


static size_t elem_get_name(const item_t elem, const size_t num, char *const buffer)
{
	if (buffer == NULL)
	{
		return 0;
	}

	size_t argc = 0;
	bool was_switch = false;

	switch (elem)
	{
		case OP_IDENTIFIER:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "IDENTIFIER");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
				case 3:
					sprintf(buffer, "id");
					break;
			}
			break;
		case OP_CONSTANT:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "CONSTANT");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
				case 3:
					sprintf(buffer, "value");
					break;
			}
			break;
		case OP_STRING:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "STRING");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;
		case OP_CALL:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "CALL");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;
		case OP_SELECT:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SELECT");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
			}
			break;
		case OP_SLICE:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SLICE");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;
		case OP_UNARY:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "UNARY");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
				case 3:
					sprintf(buffer, "operator");
					break;
			}
			break;
		case OP_BINARY:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "BINARY");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
				case 3:
					sprintf(buffer, "operator");
					break;
			}
			break;
		case OP_TERNARY:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TERNARY");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;
		case OP_LIST:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "LIST");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;

		case OP_PRINT:
			sprintf(buffer, "TPrint");
			break;
		case OP_PRINTID:
			argc = 1;
			sprintf(buffer, "TPrintid");
			break;
		case OP_PRINTF:
			sprintf(buffer, "TPrintf");
			break;
		case OP_GETID:
			argc = 1;
			sprintf(buffer, "TGetid");
			break;
		case OP_THREAD:
			sprintf(buffer, "TCREATEDIRECT");
			break;
		case OP_UPB:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "UPB");
					break;
				case 1:
					sprintf(buffer, "type");
					break;
				case 2:
					sprintf(buffer, "designation");
					break;
			}
			break;

		case OP_FUNC_DEF:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TFuncdef");
					break;
				case 1:
					sprintf(buffer, "funcn");
					break;
				case 2:
					sprintf(buffer, "maxdispl");
					break;
			}
			break;
		case OP_DECL_ARR:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TDeclarr");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
			}
			break;
		case OP_DECL_ID:
			argc = 7;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TDeclid");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "eltype");
					break;
				case 3:
					sprintf(buffer, "N");
					break;
				case 4:
					sprintf(buffer, "all");
					break;
				case 5:
					sprintf(buffer, "iniproc");
					break;
				case 6:
					sprintf(buffer, "usual");
					break;
				case 7:
					sprintf(buffer, "instuct");
					break;
			}
			break;
		case OP_BLOCK:
			sprintf(buffer, "TBegin");
			break;
		case OP_ARRAY_INIT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TBeginit");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case OP_STRUCT_INIT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TStructinit");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case OP_IF:
			argc = 1;
			sprintf(buffer, "TIf");
			break;
		case OP_WHILE:
			sprintf(buffer, "TWhile");
			break;
		case OP_DO:
			sprintf(buffer, "TDo");
			break;
		case OP_FOR:
			argc = 4;
			sprintf(buffer, "TFor");
			break;
		case OP_SWITCH:
			sprintf(buffer, "TSwitch");
			break;
		case OP_CASE:
			sprintf(buffer, "TCase");
			break;
		case OP_DEFAULT:
			sprintf(buffer, "TDefault");
			break;
		case OP_BREAK:
			sprintf(buffer, "TBreak");
			break;
		case OP_CONTINUE:
			sprintf(buffer, "TContinue");
			break;
		case OP_RETURN_VOID:
			sprintf(buffer, "TReturn");
			break;
		case OP_RETURN_VAL:
			argc = 1;
			sprintf(buffer, "TReturnval");
			break;
		case OP_GOTO:
			argc = 1;
			sprintf(buffer, "TGoto");
			break;
		case OP_NOP:
			sprintf(buffer, "NOP");
			break;
		case IC_COPY01:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY01");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY10:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY10");
					break;
				case 1:
					sprintf(buffer, "displright");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY11:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY11");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY0ST:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0ST");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY1ST:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1ST");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY0ST_ASSIGN:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0STASS");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY1ST_ASSIGN:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1STASS");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPYST:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPYST");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
				case 3:
					sprintf(buffer, "length1");
					break;
			}
			break;

		case IC_CALL1:
			argc = 1;
			sprintf(buffer, "TCall1");
			break;
		case IC_CALL2:
			argc = 1;
			sprintf(buffer, "TCall2");
			break;
		case OP_LABEL:
			argc = 1;
			sprintf(buffer, "TLabel");
			break;
		case OP_DECL_STRUCT:
			argc = 1;
			sprintf(buffer, "TStructbeg");
			break;
		case OP_DECL_STRUCT_END:
			argc = 1;
			sprintf(buffer, "TStructend");
			break;
		case IC_CREATE:
			sprintf(buffer, "TCREATE");
			break;
		case IC_CREATE_DIRECT:
			sprintf(buffer, "TCREATEDIRECT");
			break;
		case IC_EXIT:
			sprintf(buffer, "TEXIT");
			break;
		case IC_EXIT_DIRECT:
			sprintf(buffer, "TEXITDIRECT");
			break;
		case IC_MSG_SEND:
			sprintf(buffer, "TMSGSEND");
			break;
		case IC_MSG_RECEIVE:
			sprintf(buffer, "TMSGRECEIVE");
			break;
		case IC_JOIN:
			sprintf(buffer, "TJOIN");
			break;
		case IC_SLEEP:
			sprintf(buffer, "TSLEEP");
			break;
		case IC_SEM_CREATE:
			sprintf(buffer, "TSEMCREATE");
			break;
		case IC_SEM_WAIT:
			sprintf(buffer, "TSEMWAIT");
			break;
		case IC_SEM_POST:
			sprintf(buffer, "TSEMPOST");
			break;
		case IC_INIT:
			sprintf(buffer, "INITC");
			break;
		case IC_DESTROY:
			sprintf(buffer, "DESTROYC");
			break;
		case IC_GETNUM:
			sprintf(buffer, "GETNUMC");
			break;

		case IC_PRINT:
			argc = 1;
			sprintf(buffer, "PRINT");
			break;
		case IC_PRINTID:
			argc = 1;
			sprintf(buffer, "PRINTID");
			break;
		case IC_PRINTF:
			argc = 1;
			sprintf(buffer, "PRINTF");
			break;
		case IC_GETID:
			argc = 1;
			sprintf(buffer, "GETID");
			break;

		case IC_ABS:
			sprintf(buffer, "ABS");
			break;
		case IC_ABSI:
			sprintf(buffer, "ABSI");
			break;
		case IC_SQRT:
			sprintf(buffer, "SQRT");
			break;
		case IC_EXP:
			sprintf(buffer, "EXP");
			break;
		case IC_SIN:
			sprintf(buffer, "SIN");
			break;
		case IC_COS:
			sprintf(buffer, "COS");
			break;
		case IC_LOG:
			sprintf(buffer, "LOG");
			break;
		case IC_LOG10:
			sprintf(buffer, "LOG10");
			break;
		case IC_ASIN:
			sprintf(buffer, "ASIN");
			break;
		case IC_RAND:
			sprintf(buffer, "RAND");
			break;
		case IC_ROUND:
			sprintf(buffer, "ROUND");
			break;

		case IC_STRCPY:
			sprintf(buffer, "STRCPY");
			break;
		case IC_STRNCPY:
			sprintf(buffer, "STRNCPY");
			break;
		case IC_STRCAT:
			sprintf(buffer, "STRCAT");
			break;
		case IC_STRNCAT:
			sprintf(buffer, "STRNCAT");
			break;
		case IC_STRCMP:
			sprintf(buffer, "STRCMP");
			break;
		case IC_STRNCMP:
			sprintf(buffer, "STRNCMP");
			break;
		case IC_STRSTR:
			sprintf(buffer, "STRSTR");
			break;
		case IC_STRLEN:
			sprintf(buffer, "STRLENC");
			break;

		case IC_BEG_INIT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "BEGINIT");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case IC_STRUCT_WITH_ARR:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "STRUCTWITHARR");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "iniproc");
					break;
			}
			break;
		case IC_DEFARR:
			argc = 7;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "DEFARR");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "iniproc");
					break;
				case 5:
					sprintf(buffer, "usual");
					break;
				case 6:
					sprintf(buffer, "all");
					break;
				case 7:
					sprintf(buffer, "instruct");
					break;
			}
			break;
		case IC_ARR_INIT:
			argc = 4;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "ARRINIT");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "usual");
					break;
			}
			break;
		case IC_LI:
			argc = 1;
			sprintf(buffer, "LI");
			break;
		case IC_LID:
			argc = 2;
			sprintf(buffer, "LID");
			break;
		case IC_LOAD:
			argc = 1;
			sprintf(buffer, "LOAD");
			break;
		case IC_LOADD:
			argc = 1;
			sprintf(buffer, "LOADD");
			break;
		case IC_LAT:
			sprintf(buffer, "L@");
			break;
		case IC_LATD:
			sprintf(buffer, "L@f");
			break;
		case IC_LA:
			argc = 1;
			sprintf(buffer, "LA");
			break;

		case IC_STOP:
			sprintf(buffer, "STOP");
			break;
		case IC_RETURN_VAL:
			argc = 1;
			sprintf(buffer, "RETURNVAL");
			break;
		case IC_RETURN_VOID:
			sprintf(buffer, "RETURNVOID");
			break;
		case IC_B:
			argc = 1;
			sprintf(buffer, "B");
			break;
		case IC_BE0:
			argc = 1;
			sprintf(buffer, "BE0");
			break;
		case IC_BNE0:
			argc = 1;
			sprintf(buffer, "BNE0");
			break;
		case IC_SLICE:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SLICE");
					break;
				case 1:
					sprintf(buffer, "d");
					break;
			}
			break;
		case IC_SELECT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SELECT");
					break;
				case 1:
					sprintf(buffer, "field_displ");
					break;
			}
			break;
		case IC_FUNC_BEG:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "FUNCBEG");
					break;
				case 1:
					sprintf(buffer, "maxdispl");
					break;
				case 2:
					sprintf(buffer, "pc");
					break;
			}
			break;

		default:
			sprintf(buffer, "%" PRIitem, elem);
			break;
	}

	if ((num != 0 && !was_switch) || argc < num)
	{
		buffer[0] = '\0';
	}
	return argc;
}


static void double_to_io(universal_io *const io, const int64_t fst, const int64_t snd)
{
	int64_t num = (snd << 32) | (fst & 0x00000000ffffffff);
	double numdouble;
	memcpy(&numdouble, &num, sizeof(double));
	uni_printf(io, " %f\n", numdouble);
}

static size_t elem_to_io(universal_io *const io, const vector *const table, size_t i)
{
	const item_t type = vector_get(table, i++);

	char buffer[MAX_ELEM_SIZE];
	size_t argc = elem_get_name(type, 0, buffer);
	uni_printf(io, "%s", buffer);

	if (type == IC_LID)
	{
		double_to_io(io, vector_get(table, i), vector_get(table, i + 1));
		return i + 2;
	}

	for (size_t j = 1; j <= argc; j++)
	{
		elem_get_name(type, j, buffer);

		if (buffer[0] != '\0')
		{
			uni_printf(io, " %s=", buffer);
		}

		uni_printf(io, " %" PRIitem, vector_get(table, i++));
	}
	uni_printf(io, "\n");

	if (type == OP_STRING)
	{
		const size_t n = (size_t)vector_get(table, i - 1);
		for (size_t j = 0; j < n; j++)
		{
			uni_printf(io, "%" PRIitem "\n", vector_get(table, i++));
		}
	}

	return i;
}


static size_t tree_print_recursive(universal_io *const io, node *const nd, size_t index, size_t tabs)
{
	for (size_t i = 0; i < tabs; i++)
	{
		uni_printf(io, INDENT);
	}
	uni_printf(io, "tc %zu) ", index);

	const item_t type = node_get_type(nd);
	char buffer[MAX_ELEM_SIZE];
	size_t argc = elem_get_name(type, 0, buffer);
	uni_printf(io, "%s", buffer);

	if (type == IC_LID)
	{
		double_to_io(io, node_get_arg(nd, 0), node_get_arg(nd, 1));
	}
	else
	{
		size_t i = 0;
		while (i < argc && node_get_arg(nd, i) != ITEM_MAX)
		{
			elem_get_name(type, i + 1, buffer);

			if (buffer[0] != '\0')
			{
				uni_printf(io, " %s=", buffer);
			}

			uni_printf(io, " %" PRIitem, node_get_arg(nd, i++));
		}
		uni_printf(io, "\n");

		if ((node_get_arg(nd, i) != ITEM_MAX && node_get_type(nd) != OP_STRING) || i != argc)
		{
			elem_get_name(type, 0, buffer);
			//warning(NULL, node_argc, index, buffer);
		}
	}

	index += argc + 1;
	for (size_t j = 0; j < node_get_amount(nd); j++)
	{
		node child = node_get_child(nd, j);
		index = tree_print_recursive(io, &child, index, tabs + 1);
	}

	return index;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


/** Вывод таблиц и дерева */
void tables_and_tree(const char *const path
	, const vector *const identifiers
	, const vector *const modes
	, vector *const tree)
{
	universal_io io = io_create();
	if (!vector_is_correct(identifiers) || !vector_is_correct(modes) || !vector_is_correct(tree)
		|| out_set_file(&io, path))
	{
		return;
	}


	uni_printf(&io, "identab\n");
	for (size_t i = 2; i < vector_size(identifiers); i += 4)
	{
		for (size_t j = 0; j < 4; j++)
		{
			uni_printf(&io, "id %zu) %" PRIitem "\n", i + j, vector_get(identifiers, i + j));
		}
		uni_printf(&io, "\n");
	}

	uni_printf(&io, "\nmodetab\n");
	for (size_t i = 0; i < vector_size(modes); i++)
	{
		uni_printf(&io, "md %zu) %" PRIitem "\n", i, vector_get(modes, i));
	}

	uni_printf(&io, "\n\ntree\n");
	size_t i = 0;
	node nd = node_get_root(tree);
	for (size_t j = 0; j < node_get_amount(&nd); j++)
	{
		node child = node_get_child(&nd, j);
		i = tree_print_recursive(&io, &child, i, 0);
	}

	io_erase(&io);
}

/** Вывод таблиц и кодов */
void tables_and_codes(const char *const path
	, const vector *const functions
	, const vector *const processes
	, const vector *const memory)
{
	universal_io io = io_create();
	if (!vector_is_correct(functions) || !vector_is_correct(processes) || !vector_is_correct(memory)
		|| out_set_file(&io, path))
	{
		return;
	}


	uni_printf(&io, "functions\n");
	for (size_t i = 0; i < vector_size(functions); i++)
	{
		uni_printf(&io, "fun %zu) %" PRIitem "\n", i, vector_get(functions, i));
	}

	uni_printf(&io, "\n\niniprocs\n");
	for (size_t i = 0; i < vector_size(processes); i++)
	{
		uni_printf(&io, "inipr %zu) %" PRIitem "\n", i, vector_get(processes, i));
	}

	uni_printf(&io, "\n\nmem\n");
	size_t i = 0;
	while (i < vector_size(memory))
	{
		uni_printf(&io, "pc %zu) ", i);
		i = elem_to_io(&io, memory, i);
	}

	io_erase(&io);
}
