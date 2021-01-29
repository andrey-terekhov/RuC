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


static void tocode(syntax *const sx, int c)
{
	// printf("tocode tc=%zi pc=%zi) %i\n", sx->tc,
	// mem_size(sx), c);
	mem_add(sx, c);
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
				tocode(sx, _DOUBLE);
				tocode(sx, BNE0);
				stack_push(sx, (int)mem_size(sx));
				mem_increase(sx, 1);
			}
			else if (op == ADLOGAND)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BE0);
				stack_push(sx, (int)mem_size(sx));
				mem_increase(sx, 1);
			}
			else
			{
				tocode(sx, op);
				if (op == LOGOR || op == LOGAND)
				{
					mem_set(sx, stack_pop(sx), (int)mem_size(sx));
				}
				else if (op == COPY00 || op == COPYST)
				{
					tocode(sx, node_get_arg(nd, 0)); // d1
					tocode(sx, node_get_arg(nd, 1)); // d2
					tocode(sx, node_get_arg(nd, 2)); // длина
				}
				else if (op == COPY01 || op == COPY10 || op == COPY0ST || op == COPY0STASS)
				{
					tocode(sx, node_get_arg(nd, 0)); // d1
					tocode(sx, node_get_arg(nd, 1)); // длина
				}
				else if (op == COPY11 || op == COPY1ST || op == COPY1STASS)
				{
					tocode(sx, node_get_arg(nd, 0)); // длина
				}
				else if ((op >= REMASS && op <= DIVASS) || (op >= REMASSV && op <= DIVASSV)
					|| (op >= ASSR && op <= DIVASSR) || (op >= ASSRV && op <= DIVASSRV) || (op >= POSTINC && op <= DEC)
					|| (op >= POSTINCV && op <= DECV) || (op >= POSTINCR && op <= DECR) || (op >= POSTINCRV && op <= DECRV))
				{
					tocode(sx,node_get_arg(nd, 0));
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
				tocode(sx, LA);
				tocode(sx, node_get_arg(nd, 0));
			}
			break;
			case TIdenttoval:
			{
				tocode(sx, LOAD);
				tocode(sx, node_get_arg(nd, 0));
			}
			break;
			case TIdenttovald:
			{
				tocode(sx, LOADD);
				tocode(sx, node_get_arg(nd, 0));
			}
			break;
			case TAddrtoval:
				tocode(sx, LAT);
				break;
			case TAddrtovald:
				tocode(sx, LATD);
				break;
			case TConst:
			{
				tocode(sx, LI);
				tocode(sx, node_get_arg(nd, 0));
			}
			break;
			case TConstd:
			{
				tocode(sx, LID);
				tocode(sx, node_get_arg(nd, 0));
				tocode(sx, node_get_arg(nd, 1));
			}
			break;
			case TString:
			case TStringd:
			{
				tocode(sx, LI);
				const size_t reserved = mem_size(sx) + 4;
				tocode(sx, (int)reserved);
				tocode(sx, B);
				mem_increase(sx, 2);

				const int N = node_get_arg(nd, 0);
				for (int i = 0; i < N; i++)
				{
					if (operation == TString)
					{
						tocode(sx, node_get_arg(nd, i + 1));
					}
					else
					{
						tocode(sx, node_get_arg(nd, 2 * i + 1));
						tocode(sx, node_get_arg(nd, 2 * i + 2));
					}
				}

				mem_set(sx, reserved - 1, N);
				mem_set(sx, reserved - 2, (int)mem_size(sx));
			}
			break;
			case TBeginit:
			{
				const int N = node_get_arg(nd, 0);

				tocode(sx, BEGINIT);
				tocode(sx, N);

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
				tocode(sx, LOAD); // параметры - смещение идента и тип элемента
				tocode(sx, node_get_arg(nd, 0)); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				int type = node_get_arg(nd, operation == TSlice ? 0 : 1);

				expression(sx, nd, 0);
				tocode(sx, SLICE);
				tocode(sx, size_of(sx, type));
				if (type > 0 && mode_get(sx, type) == MARRAY)
				{
					tocode(sx, LAT);
				}
			}
			break;
			case TSelect:
			{
				tocode(sx, SELECT); // SELECT field_displ
				tocode(sx, node_get_arg(nd, 0));
			}
			break;
			case TPrint:
			{
				tocode(sx, PRINT);
				tocode(sx, node_get_arg(nd, 0)); // type
			}
			break;
			case TCall1:
			{
				tocode(sx, CALL1);

				const int N = node_get_arg(nd, 0);
				for (int i = 0; i < N; i++)
				{
					expression(sx, nd, 0);
				}
			}
			break;
			case TCall2:
			{
				tocode(sx, CALL2);
				tocode(sx, ident_get_displ(sx, node_get_arg(nd, 0)));
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
				tocode(sx, BE0);
				const size_t addr_else = mem_size(sx);
				mem_increase(sx, 1);

				expression(sx, nd, 0); // then
				tocode(sx, B);
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
	node cur = node_get_next(nd);
	if (node_get_type(&cur) == TStructinit)
	{
		const int N = node_get_arg(&cur, 0);
		for (int i = 0; i < N; i++)
		{
			structure(sx, &cur);
		}

		node_set_next(&cur); // TExprend
		node_copy(nd, &cur);
	}
	else
	{
		expression(sx, nd, 0);
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
			tocode(sx, STRUCTWITHARR);
			tocode(sx, old_displ);
			tocode(sx, proc_get(sx, process));
		}
		if (all) // int a = или struct{} a =
		{
			if (type > 0 && mode_get(sx, type) == MSTRUCT)
			{
				structure(sx, nd);

				tocode(sx, COPY0STASS);
				tocode(sx, old_displ);
				tocode(sx, all); // Общее количество слов
			}
			else
			{
				expression(sx, nd, 0);

				tocode(sx, type == LFLOAT ? ASSRV : ASSV);
				tocode(sx, old_displ);
			}
		}
	}
	else // Обработка массива int a[N1]...[NN] =
	{
		const int length = size_of(sx, type);

		tocode(sx, DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
		tocode(sx, all == 0 ? N : abs(N) - 1);
		tocode(sx, length);
		tocode(sx, old_displ);
		tocode(sx, proc_get(sx, process));
		tocode(sx, usual);
		tocode(sx, all);
		tocode(sx, instruction);

		if (all) // all == 1, если есть инициализация массива
		{
			expression(sx, nd, 0);

			tocode(sx, ARRINIT); // ARRINIT N d all displ usual
			tocode(sx, abs(N));
			tocode(sx, length);
			tocode(sx, old_displ);
			tocode(sx, usual);	// == 0 с пустыми границами
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
			tocode(sx, B);
			tocode(sx, 0);
			proc_set(sx, node_get_arg(nd, 0), (int)mem_size(sx));
		}
		break;
		case TStructend:
		{
			const int num_proc = node_get_arg(nd, 0);

			tocode(sx, STOP);
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
			tocode(sx, CREATEDIRECTC);
			sx->max_threads++;
			break;
		case EXITDIRECTC:
		case EXITC:
			tocode(sx, EXITC);
			break;
		case TBegin:
			block(sx, nd, context);
			break;
		case TIf:
		{
			const int ref_else = node_get_arg(nd, 0);

			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			tocode(sx, BE0);
			size_t addr = mem_size(sx);
			mem_increase(sx, 1);
			statement(sx, nd, context);

			if (ref_else)
			{
				node_set_next(nd);
				mem_set(sx, addr, (int)mem_size(sx) + 2);
				tocode(sx, B);
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

			tocode(sx, BE0);
			context->addr_break = mem_size(sx);
			mem_add(sx, 0);
			statement(sx, nd, context);

			addr_begin_condition(sx, context, addr);
			tocode(sx, B);
			tocode(sx, (int)addr);
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
			tocode(sx, BNE0);
			tocode(sx, (int)addr);
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
				tocode(sx, BE0);
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

			tocode(sx, B);
			tocode(sx, (int)initad);
			addr_end_break(sx, context);

			context->addr_break = old_break;
			context->addr_cond = old_cond;
		}
		break;
		case TGoto:
		{
			tocode(sx, B);

			const int id_sign = node_get_arg(nd, 0);
			const size_t id = id_sign > 0 ? id_sign : -id_sign;
			const int addr = ident_get_displ(sx, id);

			if (addr > 0) // метка уже описана
			{
				tocode(sx, addr);
			}
			else // метка еще не описана
			{
				ident_set_displ(sx, id, -(int)mem_size(sx));

				// первый раз встретился переход на еще не описанную метку или нет
				tocode(sx, id_sign < 0 ? 0 : addr);
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
			tocode(sx, _DOUBLE);
			expression(sx, nd, 0);
			node_set_next(nd); // TExprend

			tocode(sx, EQEQ);
			tocode(sx, BE0);
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
			tocode(sx, B);
			mem_add(sx, (int)context->addr_break);
			context->addr_break = mem_size(sx) - 1;
		}
		break;
		case TContinue:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->addr_cond);
			context->addr_cond = mem_size(sx) - 1;
		}
		break;
		case TReturnvoid:
			tocode(sx, RETURNVOID);
			break;
		case TReturnval:
		{
			const int value = node_get_arg(nd, 0);
			expression(sx, nd, 0);

			tocode(sx, RETURNVAL);
			tocode(sx, value);
		}
		break;
		case TPrintid:
		{
			tocode(sx, PRINTID);
			tocode(sx, node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		case TPrintf:
		{
			tocode(sx, PRINTF);
			tocode(sx, node_get_arg(nd, 0)); // общий размер того, что надо вывести
		}
		break;
		case TGetid:
		{
			tocode(sx, GETID);
			tocode(sx, node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		case SETMOTOR:
		{
			expression(sx, nd, 0);
			expression(sx, nd, 0);

			tocode(sx, SETMOTORC);
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

	node root = node_get_root(sx);
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
				tocode(sx, FUNCBEG);
				tocode(sx, max_displ);

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

	tocode(sx, CALL1);
	tocode(sx, CALL2);
	tocode(sx, ident_get_displ(sx, sx->ref_main));
	tocode(sx, STOP);
	return 0;
}

/** Вывод таблиц в файл */
void output_export(universal_io *const io, const syntax *const sx)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %i %zi\n", mem_size(sx), sx->funcnum, sx->id,
				   sx->rp, sx->md, sx->maxdisplg, sx->max_threads);

	for (size_t i = 0; i < mem_size(sx); i++)
	{
		uni_printf(io, "%i ", mem_get(sx, i));
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->funcnum; i++)
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
