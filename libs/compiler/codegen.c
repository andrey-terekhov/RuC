/*
 *	Copyright 2015 Andrey Terekhov
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

#include "codegen.h"
#include <stdlib.h>
#include "defs.h"
#include "errors.h"
#include "tree.h"
#include "uniprinter.h"


typedef struct address
{
	size_t addr_cond;
	size_t addr_case;
	size_t addr_break;
} address;


static void block(syntax *const sx, node *const nd, address *const context);


static void addr_begin_condition(syntax *const sx, address *const context, const size_t addr)
{
	while (context->addr_cond != addr)
	{
		const size_t ref = mem_get(sx, context->addr_cond);
		mem_set(sx, context->addr_cond, (int)addr);
		context->addr_cond = ref;
	}
}

static void addr_end_condition(syntax *const sx, address *const context)
{
	while (context->addr_cond)
	{
		const size_t ref = mem_get(sx, context->addr_cond);
		mem_set(sx, context->addr_cond, (int)mem_size(sx));
		context->addr_cond = ref;
	}
}

static void addr_end_break(syntax *const sx, address *const context)
{
	while (context->addr_break)
	{
		const size_t ref = mem_get(sx, context->addr_break);
		mem_set(sx, context->addr_break, (int)mem_size(sx));
		context->addr_break = ref;
	}
}


static void final_operation(syntax *const sx, node *const nd)
{
	int op = node_get_type(nd);
	while (op > 9000)
	{
		if (op != NOP)
		{
			if (op == ADLOGOR)
			{
				mem_add(sx, _DOUBLE);
				mem_add(sx, BNE0);
				stack_push(sx, (int)mem_size(sx));
				mem_increase(sx, 1);
			}
			else if (op == ADLOGAND)
			{
				mem_add(sx, _DOUBLE);
				mem_add(sx, BE0);
				stack_push(sx, (int)mem_size(sx));
				mem_increase(sx, 1);
			}
			else
			{
				mem_add(sx, op);
				if (op == LOGOR || op == LOGAND)
				{
					mem_set(sx, stack_pop(sx), (int)mem_size(sx));
				}
				else if (op == COPY00 || op == COPYST)
				{
					mem_add(sx, node_get_arg(nd, 0)); // d1
					mem_add(sx, node_get_arg(nd, 1)); // d2
					mem_add(sx, node_get_arg(nd, 2)); // длина
				}
				else if (op == COPY01 || op == COPY10 || op == COPY0ST || op == COPY0STASS)
				{
					mem_add(sx, node_get_arg(nd, 0)); // d1
					mem_add(sx, node_get_arg(nd, 1)); // длина
				}
				else if (op == COPY11 || op == COPY1ST || op == COPY1STASS)
				{
					mem_add(sx, node_get_arg(nd, 0)); // длина
				}
				else if ((op >= REMASS && op <= DIVASS) || (op >= REMASSV && op <= DIVASSV)
					|| (op >= ASSR && op <= DIVASSR) || (op >= ASSRV && op <= DIVASSRV) || (op >= POSTINC && op <= DEC)
					|| (op >= POSTINCV && op <= DECV) || (op >= POSTINCR && op <= DECR) || (op >= POSTINCRV && op <= DECRV))
				{
					mem_add(sx,node_get_arg(nd, 0));
				}
			}
		}

		node_set_next(nd);
		op = node_get_type(nd);
	}
}

/**
 *	Expression generation
 *
 *	@param	sx		Syntax structure
 *	@param	mode	@c -1 for expression on the same node,
 *					@c  0 for usual expression,
 *					@c  1 for expression in condition
 */
static void expression(syntax *const sx, node *const nd, int mode)
{
	if (mode != -1)
	{
		node_set_next(nd);
	}

	while (node_get_type(nd) != TExprend)
	{
		const int operation = node_get_type(nd);
		int was_operation = 1;

		switch (operation)
		{
			case TIdent:
				break;
			case TIdenttoaddr:
			{
				mem_add(sx, LA);
				mem_add(sx, node_get_arg(nd, 0));
			}
			break;
			case TIdenttoval:
			{
				mem_add(sx, LOAD);
				mem_add(sx, node_get_arg(nd, 0));
			}
			break;
			case TIdenttovald:
			{
				mem_add(sx, LOADD);
				mem_add(sx, node_get_arg(nd, 0));
			}
			break;
			case TAddrtoval:
				mem_add(sx, LAT);
				break;
			case TAddrtovald:
				mem_add(sx, LATD);
				break;
			case TConst:
			{
				mem_add(sx, LI);
				mem_add(sx, node_get_arg(nd, 0));
			}
			break;
			case TConstd:
			{
				mem_add(sx, LID);
				mem_add(sx, node_get_arg(nd, 0));
				mem_add(sx, node_get_arg(nd, 1));
			}
			break;
			case TString:
			case TStringd:
			{
				mem_add(sx, LI);
				const size_t reserved = mem_size(sx) + 4;
				mem_add(sx, (int)reserved);
				mem_add(sx, B);
				mem_increase(sx, 2);

				const int N = node_get_arg(nd, 0);
				for (int i = 0; i < N; i++)
				{
					if (operation == TString)
					{
						mem_add(sx, node_get_arg(nd, i + 1));
					}
					else
					{
						mem_add(sx, node_get_arg(nd, 2 * i + 1));
						mem_add(sx, node_get_arg(nd, 2 * i + 2));
					}
				}

				mem_set(sx, reserved - 1, N);
				mem_set(sx, reserved - 2, (int)mem_size(sx));
			}
			break;
			case TBeginit:
			{
				const int N = node_get_arg(nd, 0);

				mem_add(sx, BEGINIT);
				mem_add(sx, N);

				for (int i = 0; i < N; i++)
				{
					expression(sx, nd, 0);
				}
			}
			break;
			case TStructinit:
			{
				const int N = node_get_arg(nd, 0);
				for (int i = 0; i < N; i++)
				{
					expression(sx, nd, 0);
				}
			}
			break;
			case TSliceident:
			{
				mem_add(sx, LOAD); // параметры - смещение идента и тип элемента
				mem_add(sx, node_get_arg(nd, 0)); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				int type = node_get_arg(nd, operation == TSlice ? 0 : 1);

				expression(sx, nd, 0);
				mem_add(sx, SLICE);
				mem_add(sx, size_of(sx, type));
				if (type > 0 && mode_get(sx, type) == MARRAY)
				{
					mem_add(sx, LAT);
				}
			}
			break;
			case TSelect:
			{
				mem_add(sx, SELECT); // SELECT field_displ
				mem_add(sx, node_get_arg(nd, 0));
			}
			break;
			case TPrint:
			{
				mem_add(sx, PRINT);
				mem_add(sx, node_get_arg(nd, 0)); // type
			}
			break;
			case TCall1:
			{
				mem_add(sx, CALL1);

				const int N = node_get_arg(nd, 0);
				for (int i = 0; i < N; i++)
				{
					expression(sx, nd, 0);
				}
			}
			break;
			case TCall2:
			{
				mem_add(sx, CALL2);
				mem_add(sx, ident_get_displ(sx, node_get_arg(nd, 0)));
			}
			break;
			default:
				was_operation = 0;
				break;
		}

		if (was_operation)
		{
			node_set_next(nd);
		}

		final_operation(sx, nd);

		if (node_get_type(nd) == TCondexpr)
		{
			if (mode == 1)
			{
				return;
			}

			size_t addr = 0;
			do
			{
				mem_add(sx, BE0);
				const size_t addr_else = mem_size(sx);
				mem_increase(sx, 1);

				expression(sx, nd, 0); // then
				mem_add(sx, B);
				mem_add(sx, (int)addr);
				addr = mem_size(sx) - 1;
				mem_set(sx, addr_else, (int)mem_size(sx));

				expression(sx, nd, 1); // else или cond
			} while (node_get_type(nd) == TCondexpr);

			while (addr)
			{
				int ref = mem_get(sx, addr);
				mem_set(sx, addr, (int)mem_size(sx));
				addr = ref;
			}

			final_operation(sx, nd);
		}
	}
}

static void structure(syntax *const sx, node *const nd)
{
	if (node_get_type(nd) == TStructinit)
	{
		const int N = node_get_arg(nd, 0);
		node_set_next(nd);

		for (int i = 0; i < N; i++)
		{
			structure(sx, nd);
			node_set_next(nd); // TExprend
		}
	}
	else
	{
		expression(sx, nd, -1);
	}
}

static void identifier(syntax *const sx, node *const nd)
{
	const int old_displ = node_get_arg(nd, 0);
	const int type = node_get_arg(nd, 1);
	const int N = node_get_arg(nd, 2);

	/*
	*	@param	all		Общее кол-во слов в структуре:
	*						@c 0 нет инициализатора,
	*						@c 1 есть инициализатор,
	*						@c 2 есть инициализатор только из строк
	*/
	const int all = node_get_arg(nd, 3);
	const int process = node_get_arg(nd, 4);

	/*
	*	@param	usual	Для массивов:
	*						@c 0 с пустыми границами,
	*						@c 1 без пустых границ
	*/
	const int usual = node_get_arg(nd, 5);
	const int instruction = node_get_arg(nd, 6);


	if (N == 0) // Обычная переменная int a; или struct point p;
	{
		if (process)
		{
			mem_add(sx, STRUCTWITHARR);
			mem_add(sx, old_displ);
			mem_add(sx, proc_get(sx, process));
		}
		if (all) // int a = или struct{} a =
		{
			if (type > 0 && mode_get(sx, type) == MSTRUCT)
			{
				node_set_next(nd);
				structure(sx, nd);

				mem_add(sx, COPY0STASS);
				mem_add(sx, old_displ);
				mem_add(sx, all); // Общее количество слов
			}
			else
			{
				expression(sx, nd, 0);

				mem_add(sx, type == LFLOAT ? ASSRV : ASSV);
				mem_add(sx, old_displ);
			}
		}
	}
	else // Обработка массива int a[N1]...[NN] =
	{
		const int length = size_of(sx, type);

		mem_add(sx, DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
		mem_add(sx, all == 0 ? N : abs(N) - 1);
		mem_add(sx, length);
		mem_add(sx, old_displ);
		mem_add(sx, proc_get(sx, process));
		mem_add(sx, usual);
		mem_add(sx, all);
		mem_add(sx, instruction);

		if (all) // all == 1, если есть инициализация массива
		{
			expression(sx, nd, 0);

			mem_add(sx, ARRINIT); // ARRINIT N d all displ usual
			mem_add(sx, abs(N));
			mem_add(sx, length);
			mem_add(sx, old_displ);
			mem_add(sx, usual);	// == 0 с пустыми границами
								// == 1 без пустых границ и без инициализации
		}
	}
}

static int declaration(syntax *const sx, node *const nd)
{
	switch (node_get_type(nd))
	{
		case TDeclarr:
		{
			const int N = node_get_arg(nd, 0);
			for (int i = 0; i < N; i++)
			{
				expression(sx, nd, 0);
			}
		}
		break;
		case TDeclid:
			identifier(sx, nd);
			break;

		case TStructbeg:
		{
			mem_add(sx, B);
			mem_add(sx, 0);
			proc_set(sx, node_get_arg(nd, 0), (int)mem_size(sx));
		}
		break;
		case TStructend:
		{
			const int num_proc = node_get_arg(nd, 0);

			mem_add(sx, STOP);
			mem_set(sx, proc_get(sx, num_proc) - 1, (int)mem_size(sx));
		}
		break;

		default:
			return -1;
	}

	return 0;
}

static void statement(syntax *const sx, node *const nd, address *const context)
{
	switch (node_get_type(nd))
	{
		case NOP:
			break;
		case CREATEDIRECTC:
			mem_add(sx, CREATEDIRECTC);
			sx->max_threads++;
			break;
		case EXITDIRECTC:
		case EXITC:
			mem_add(sx, EXITC);
			break;
		case TBegin:
			block(sx, nd, context);
			break;
		case TIf:
		{
			const int ref_else = node_get_arg(nd, 0);

			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(sx, BE0);
			size_t addr = mem_size(sx);
			mem_increase(sx, 1);
			statement(sx, nd, context);

			if (ref_else)
			{
				node_set_next(nd);
				mem_set(sx, addr, (int)mem_size(sx) + 2);
				mem_add(sx, B);
				addr = mem_size(sx);
				mem_increase(sx, 1);
				statement(sx, nd, context);
			}
			mem_set(sx, addr, (int)mem_size(sx));
		}
		break;
		case TWhile:
		{
			const size_t old_break = context->addr_break;
			const size_t old_cond = context->addr_cond;
			const size_t addr = mem_size(sx);

			context->addr_cond = addr;
			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(sx, BE0);
			context->addr_break = mem_size(sx);
			mem_add(sx, 0);
			statement(sx, nd, context);

			addr_begin_condition(sx, context, addr);
			mem_add(sx, B);
			mem_add(sx, (int)addr);
			addr_end_break(sx, context);

			context->addr_break = old_break;
			context->addr_cond = old_cond;
		}
		break;
		case TDo:
		{
			const size_t old_break = context->addr_break;
			const size_t old_cond = context->addr_cond;
			const size_t addr = mem_size(sx);

			context->addr_cond = 0;
			context->addr_break = 0;

			node_set_next(nd);
			statement(sx, nd, context);
			addr_end_condition(sx, context);

			expression(sx, nd, 0);
			mem_add(sx, BNE0);
			mem_add(sx, (int)addr);
			addr_end_break(sx, context);

			context->addr_break = old_break;
			context->addr_cond = old_cond;
		}
		break;
		case TFor:
		{
			const int ref_from = node_get_arg(nd, 0);
			const int ref_cond = node_get_arg(nd, 1);
			const int ref_incr = node_get_arg(nd, 2);

			node incr;
			node_copy(&incr, nd);
			size_t child_stmt = 0;

			if (ref_from)
			{
				expression(sx, &incr, 0); // initialization
				child_stmt++;
			}

			const size_t old_break = context->addr_break;
			const size_t old_cond = context->addr_cond;
			context->addr_cond = 0;
			context->addr_break = 0;

			size_t initad = mem_size(sx);
			if (ref_cond)
			{
				expression(sx, &incr, 0); // condition
				mem_add(sx, BE0);
				context->addr_break = mem_size(sx);
				mem_add(sx, 0);
				child_stmt++;
			}

			if (ref_incr)
			{
				child_stmt++;
			}

			node stmt = node_get_child(nd, child_stmt);
			statement(sx, &stmt, context);
			addr_end_condition(sx, context);

			if (ref_incr)
			{
				expression(sx, &incr, 0); // increment
			}
			node_copy(nd, &stmt);

			mem_add(sx, B);
			mem_add(sx, (int)initad);
			addr_end_break(sx, context);

			context->addr_break = old_break;
			context->addr_cond = old_cond;
		}
		break;
		case TGoto:
		{
			mem_add(sx, B);

			const int id_sign = node_get_arg(nd, 0);
			const size_t id = id_sign > 0 ? id_sign : -id_sign;
			const int addr = ident_get_displ(sx, id);

			if (addr > 0) // метка уже описана
			{
				mem_add(sx, addr);
			}
			else // метка еще не описана
			{
				ident_set_displ(sx, id, -(int)mem_size(sx));

				// первый раз встретился переход на еще не описанную метку или нет
				mem_add(sx, id_sign < 0 ? 0 : addr);
			}
		}
		break;
		case TLabel:
		{
			const int id = node_get_arg(nd, 0);
			int addr = ident_get_displ(sx, id);

			if (addr < 0) // были переходы на метку
			{
				while (addr) // проставить ссылку на метку во всех ранних переходах
				{
					int ref = mem_get(sx, -addr);
					mem_set(sx, -addr, (int)mem_size(sx));
					addr = ref;
				}
			}
			ident_set_displ(sx, id, (int)mem_size(sx));
		}
		break;
		case TSwitch:
		{
			const size_t old_break = context->addr_break;
			const size_t old_case = context->addr_case;
			context->addr_break = 0;
			context->addr_case = 0;

			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			statement(sx, nd, context);
			if (context->addr_case > 0)
			{
				mem_set(sx, context->addr_case, (int)mem_size(sx));
			}
			addr_end_break(sx, context);

			context->addr_case = old_case;
			context->addr_break = old_break;
		}
		break;
		case TCase:
		{
			if (context->addr_case)
			{
				mem_set(sx, context->addr_case, (int)mem_size(sx));
			}
			mem_add(sx, _DOUBLE);
			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(sx, EQEQ);
			mem_add(sx, BE0);
			context->addr_case = mem_size(sx);
			mem_increase(sx, 1);
			statement(sx, nd, context);
		}
		break;
		case TDefault:
		{
			if (context->addr_case)
			{
				mem_set(sx, context->addr_case, (int)mem_size(sx));
			}
			context->addr_case = 0;

			node_set_next(nd);
			statement(sx, nd, context);
		}
		break;
		case TBreak:
		{
			mem_add(sx, B);
			mem_add(sx, (int)context->addr_break);
			context->addr_break = mem_size(sx) - 1;
		}
		break;
		case TContinue:
		{
			mem_add(sx, B);
			mem_add(sx, (int)context->addr_cond);
			context->addr_cond = mem_size(sx) - 1;
		}
		break;
		case TReturnvoid:
			mem_add(sx, RETURNVOID);
			break;
		case TReturnval:
		{
			const int value = node_get_arg(nd, 0);
			expression(sx, nd, 0);

			mem_add(sx, RETURNVAL);
			mem_add(sx, value);
		}
		break;
		case TPrintid:
		{
			mem_add(sx, PRINTID);
			mem_add(sx, node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		case TPrintf:
		{
			mem_add(sx, PRINTF);
			mem_add(sx, node_get_arg(nd, 0)); // общий размер того, что надо вывести
		}
		break;
		case TGetid:
		{
			mem_add(sx, GETID);
			mem_add(sx, node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		case SETMOTOR:
		{
			expression(sx, nd, 0);
			expression(sx, nd, 0);

			mem_add(sx, SETMOTORC);
		}
		break;
		default:
			if (declaration(sx, nd))
			{
				expression(sx, nd, -1);
			}
			break;
	}
}

static void block(syntax *const sx, node *const nd, address *const context)
{
	node_set_next(nd); // TBegin
	while (node_get_type(nd) != TEnd)
	{
		statement(sx, nd, context);
		node_set_next(nd);
	}
}

/** Генерация кодов */
static int codegen(syntax *const sx)
{
	address context;

	node root = node_get_root(&sx->tree);
	while (node_set_next(&root) == 0)
	{
		switch (node_get_type(&root))
		{
			case TFuncdef:
			{
				const int ref_ident = node_get_arg(&root, 0);
				const int max_displ = node_get_arg(&root, 1);
				const int func = ident_get_displ(sx, ref_ident);

				func_set(sx, func, mem_size(sx));
				mem_add(sx, FUNCBEG);
				mem_add(sx, max_displ);

				const size_t old_pc = mem_size(sx);
				mem_increase(sx, 1);

				node_set_next(&root);
				block(sx, &root, &context);

				mem_set(sx, old_pc, (int)mem_size(sx));
			}
			break;

			case NOP:
			case TEnd:
				break;

			default:
				if (declaration(sx, &root))
				{
					error(NULL, node_unexpected, node_get_type(&root));
					return -1;
				}
				break;
		}
	}

	mem_add(sx, CALL1);
	mem_add(sx, CALL2);
	mem_add(sx, ident_get_displ(sx, sx->ref_main));
	mem_add(sx, STOP);
	return 0;
}

/** Вывод таблиц в файл */
void output_export(universal_io *const io, const syntax *const sx)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %i %zi\n", mem_size(sx), vector_size(&sx->functions), sx->id,
				   sx->rp, sx->md, sx->maxdisplg, sx->max_threads);

	for (size_t i = 0; i < mem_size(sx); i++)
	{
		uni_printf(io, "%" PRIitem " ", mem_get(sx, i));
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < vector_size(&sx->functions); i++)
	{
		uni_printf(io, "%zi ", func_get(sx, i));
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->id; i++)
	{
		uni_printf(io, "%i ", sx->identab[i]);
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->rp; i++)
	{
		uni_printf(io, "%i ", sx->reprtab[i]);
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->md; i++)
	{
		uni_printf(io, "%i ", mode_get(sx, i));
	}
	uni_printf(io, "\n");
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_vm(universal_io *const io, syntax *const sx)
{
	if (!out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	int ret = codegen(sx);
	if (!ret)
	{
		output_export(io, sx);
	}

	return ret;
}
