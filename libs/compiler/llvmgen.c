/*
 *	Copyright 2021 Andrey Terekhov, Ivan S. Arkhipov
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

#include "llvmgen.h"
#include "errors.h"
#include "llvmopt.h"
#include "tree.h"
#include "uniprinter.h"


typedef enum ANSWER
{
	AREG,								/**< Ответ находится в регистре */
	ACONST,								/**< Ответ является константой */
} answer_t;

typedef enum LOCATION
{
	LREG,								/**< Переменная находится в регистре */
	LMEM,								/**< Переменная находится в памяти */
} location_t;

typedef struct information
{
	universal_io *io;					/**< Вывод */
	syntax *sx;							/**< Структура syntax с таблицами */

	item_t string_num;					/**< Номер строки */
	item_t register_num;				/**< Номер регистра */

	item_t request_reg;					/**< Регистр на запрос */
	location_t variable_location;		/**< Расположение переменной */

	item_t answer_reg;					/**< Регистр с ответом */
	item_t answer_const;				/**< Константа с ответом */
	answer_t answer_type;				/**< Тип ответа */
} information;


static void expression(information *const info, node *const nd);
static void block(information *const info, node *const nd);


static void operand(information *const info, node *const nd)
{
	if (node_get_type(nd) == NOP || node_get_type(nd) == ADLOGOR || node_get_type(nd) == ADLOGAND)
	{
		node_set_next(nd);
	}

	switch (node_get_type(nd))
	{
		case TIdent:
		case TSelect:
		case TIdenttovald:
		case TIdenttoaddr:
		case TConstd:
			node_set_next(nd);
			break;
		case TIdenttoval:
		{
			const item_t displ = node_get_arg(nd, 0);

			uni_printf(info->io, " %%.%" PRIitem " = load i32, i32* %%var.%" PRIitem ", align 4\n"
				, info->register_num, displ);
			info->answer_reg = info->register_num++;
			info->answer_type = AREG;
			node_set_next(nd);
		}
		break;
		case TConst:
		{
			const item_t num = node_get_arg(nd, 0);

			if (info->variable_location == LMEM)
			{
				uni_printf(info->io, " store i32 %" PRIitem ", i32* %%var.%" PRIitem ", align 4\n"
					, num, info->request_reg);
				info->answer_type = AREG;
			}
			else
			{
				info->answer_type = ACONST;
				info->answer_const = num;
			}

			node_set_next(nd);
		}
		break;
		case TString:
			node_set_next(nd);
			break;
		case TSliceident:
		{
			node_set_next(nd);
			expression(info, nd);

			while (node_get_type(nd) == TSlice)
			{
				node_set_next(nd);
				expression(info, nd);
			}
		}
		break;
		case TCall1:
		{
			const item_t args = node_get_arg(nd, 0);

			node_set_next(nd);
			for (item_t i = 0; i < args; i++)
			{
				expression(info, nd);
			}
			node_set_next(nd); // TCall2
		}
		break;
		case TBeginit:
		{
			// здесь будет печать llvm с инициализацией массивов
			const item_t N = node_get_arg(nd, 0);

			node_set_next(nd);
			for (item_t i = 0; i < N; i++)
			{
				expression(info, nd);
			}
		}
		break;
		case TStructinit:
		{
			// здесь будет печать llvm с инициализацией структур
			const item_t N = node_get_arg(nd, 0);

			node_set_next(nd);
			for (item_t i = 0; i < N; i++)
			{
				expression(info, nd);
			}
		}
		break;
		default:
			node_set_next(nd);
			break;
	}
}

static void expression(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		// бинарные операции
		// пока не все, будут вводиться по мере тестирования
		case LPLUS:
		case LMINUS:
		case LMULT:
		case LDIV:
		{
			node_set_next(nd);
			expression(info, nd);
			expression(info, nd);
		}
		break;

		// унарные операции
		// пока не все, будут вводиться по мере тестирования
		case ASS:
		case PLUSASS:
		case MINUSASS:
		case MULTASS:
		case DIVASS:

		case ASSV:
		case PLUSASSV:
		case MINUSASSV:
		case MULTASSV:
		case DIVASSV:
		{
			node_set_next(nd);
			expression(info, nd);
		}
		break;
		default:
			operand(info, nd);
			break;
	}

	if (node_get_type(nd) == TExprend)
	{
		node_set_next(nd);
	}
}

static void statement(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case TBegin:
		{
			node_set_next(nd);
			block(info, nd);
		}
		break;
		case TIf:
		{
			const item_t ref_else = node_get_arg(nd, 0);

			node_set_next(nd);
			expression(info, nd);
			statement(info, nd);
			if (ref_else)
			{
				statement(info, nd);
			}
		}
		break;
		case TSwitch:
		case TCase:
		case TDefault:
		case TWhile:
		{
			node_set_next(nd);
			expression(info, nd);
			statement(info, nd);
		}
		break;
		case TDo:
		{
			node_set_next(nd);
			statement(info, nd);
			expression(info, nd);
		}
		break;
		case TFor:
		{
			const item_t ref_from = node_get_arg(nd, 0);
			const item_t ref_cond = node_get_arg(nd, 1);
			const item_t ref_incr = node_get_arg(nd, 2);

			node_set_next(nd);
			if (ref_from)
			{
				expression(info, nd);
			}
			if (ref_cond)
			{
				expression(info, nd);
			}
			if (ref_incr)
			{
				expression(info, nd);
			}
			statement(info, nd);
		}
		break;
		case TLabel:
		{
			node_set_next(nd);
			statement(info, nd);
		}
		break;
		case TBreak:
		case TContinue:
		case TReturnvoid:
		case TGoto:
			node_set_next(nd);
			break;
		case TReturnval:
		{
			node_set_next(nd);
			info->variable_location = LREG;
			expression(info, nd);
			if (info->answer_type == ACONST)
			{
				uni_printf(info->io, " ret i32 %" PRIitem "\n", info->answer_const);
			}
		}
		break;
		case TGetid:
			// здесь будет печать llvm для ввода
			node_set_next(nd);
			break;
		case TPrintid:
			// здесь будет печать llvm для вывода
			node_set_next(nd);
			break;
		case TPrintf:
		{
			const item_t N = node_get_arg(nd, 0);
			item_t args[128];

			node_set_next(nd);
			const item_t string_length = node_get_arg(nd, 0);
			node_set_next(nd); // TString
			node_set_next(nd); // TExprend
			for (item_t i = 0; i < N; i++)
			{
				info->variable_location = LREG;
				expression(info, nd);
				args[i] = info->answer_reg;
			}

			uni_printf(info->io, " %%.%" PRIitem " = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
				"([%" PRIitem " x i8], [%" PRIitem " x i8]* @.str%" PRIitem ", i32 0, i32 0)"
				, info->register_num
				, string_length + 1
				, string_length + 1
				, info->string_num);
			
			info->register_num++;
			info->string_num++;

			for (item_t i = 0; i < N; i++)
			{
				uni_printf(info->io, ", i32 signext %%.%" PRIitem, args[i]);
			}

			uni_printf(info->io, ")\n");
		}
		break;
		default:
			expression(info, nd);
			break;
	}
}

static void init(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case TBeginit:
		case TStructinit:
		{
			const item_t N = node_get_arg(nd, 0);

			node_set_next(nd);
			for (item_t i = 0; i < N; i++)
			{
				expression(info, nd);
			}
		}
		break;
		default:
			expression(info, nd);
			break;
	}
}

static void block(information *const info, node *const nd)
{
	while (node_get_type(nd) != TEnd)
	{
		switch (node_get_type(nd))
		{
			case TDeclarr:
			{
				const item_t N = node_get_arg(nd, 0);

				node_set_next(nd);
				for (item_t i = 0; i < N; i++)
				{
					expression(info, nd);
				}
			}
			break;
			case TDeclid:
			{
				const item_t displ = node_get_arg(nd, 0);
				const item_t elem_type = node_get_arg(nd, 1);
				const item_t N = node_get_arg(nd, 2);
				const item_t all = node_get_arg(nd, 3);

				if (N == 0) // обычная переменная int a; или struct point p;
				{
					if (elem_type == LINT)
					{
						uni_printf(info->io, " %%var.%" PRIitem " = alloca i32, align 4\n", displ);
						info->variable_location = LMEM;
						info->request_reg = displ;
					}
				}

				node_set_next(nd);
				if (all)
				{
					init(info, nd);
				}
			}
			break;
			case NOP:
			case TStructbeg:
			case TStructend:
				node_set_next(nd);
				break;
			default:
				statement(info, nd);
				break;
		}
	}
	node_set_next(nd); // TEnd
}

static int codegen(universal_io *const io, syntax *const sx)
{
	information info;
	info.io = io;
	info.sx = sx;
	info.string_num = 1;
	info.register_num = 1;
	info.variable_location = LREG;
	info.request_reg = 0;
	info.answer_reg = 0;

	node root = node_get_root(&sx->tree);
	while (node_set_next(&root) == 0)
	{
		switch (node_get_type(&root))
		{
			case TFuncdef:
			{
				const size_t ref_ident = (size_t)node_get_arg(&root, 0) / 4;

				if (ident_get_mode(info.sx, ref_ident) == LMAIN)
				{
					uni_printf(info.io, "define i32 @main(");
				}
				uni_printf(info.io, ") {\n");

				node_set_next(&root); // TBegin
				node_set_next(&root);
				block(&info, &root);
				uni_printf(info.io, "}\n\n");
			}
			break;
			default:
			{
				system_error(node_unexpected, node_get_type(&root));
				return -1;
			}
		}
	}
	
	return 0;
}


/*
 *	 __	 __   __	 ______   ______	 ______	 ______   ______	 ______	 ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	\ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	 \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_llvm(const workspace *const ws, universal_io *const io, syntax *const sx)
{
	if (optimize_for_llvm(ws, io, sx))
	{
		return -1;
	}

	return codegen(io, sx);
}
