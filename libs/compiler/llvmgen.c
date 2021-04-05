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
#include "codes.h"
#include "errors.h"
#include "llvmopt.h"
#include "tree.h"
#include "uniprinter.h"


#define ARITHMETIC_NUMBER 10


typedef enum ANSWER
{
	AREG,								/**< Ответ находится в регистре */
	ACONST,								/**< Ответ является константой */
} answer_t;

typedef enum LOCATION
{
	LREG,								/**< Переменная находится в регистре */
	LMEM,								/**< Переменная находится в памяти */
	LFREE,								/**< Свободный запрос на значение */
} location_t;

typedef enum OPERATION
{
	UNARY_OPERATION,
	BINARY_OPERATION,
	NOT_OPERATION,
} operation_t;

typedef enum ARITHMETIC
{
	add_llvm,
	sub_llvm,
	mul_llvm,

	sdiv_llvm,
	srem_llvm,
	shl_llvm,
	ashr_llvm,
	and_llvm,
	xor_llvm,
	or_llvm,
} arithmetic_t;

typedef struct information
{
	universal_io *io;							/**< Вывод */
	syntax *sx;									/**< Структура syntax с таблицами */
	char *arith[ARITHMETIC_NUMBER];				/**< Массив с арифметическими операциями */

	item_t string_num;							/**< Номер строки */
	item_t register_num;						/**< Номер регистра */

	item_t request_reg;							/**< Регистр на запрос */
	location_t variable_location;				/**< Расположение переменной */

	item_t answer_reg;							/**< Регистр с ответом */
	item_t answer_const;						/**< Константа с ответом */
	answer_t answer_type;						/**< Тип ответа */
} information;


static void expression(information *const info, node *const nd);
static void block(information *const info, node *const nd);


static void tocode_arithmetic_reg_reg(information *const info, item_t result, 
										arithmetic_t operation, item_t argument1, item_t argument2)
{
	if (operation < sdiv_llvm)
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s nsw i32 %%.%" PRIitem ", %%.%" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	else
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s i32 %%.%" PRIitem ", %%.%" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	
}

static void tocode_arithmetic_reg_const(information *const info, item_t result, 
										arithmetic_t operation, item_t argument1, item_t argument2)
{
	if (operation < sdiv_llvm)
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s nsw i32 %%.%" PRIitem ", %" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	else
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s i32 %%.%" PRIitem ", %" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	
}

static void tocode_arithmetic_const_reg(information *const info, item_t result, 
										arithmetic_t operation, item_t argument1, item_t argument2)
{
	if (operation < sdiv_llvm)
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s nsw i32 %" PRIitem ", %%.%" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	else
	{
		uni_printf(info->io, " %%.%" PRIitem " = %s i32 %" PRIitem ", %%.%" PRIitem "\n"
			, result, info->arith[operation], argument1, argument2);
	}
	
}


static operation_t operation_type(node *const nd)
{
	switch (node_get_type(nd))
	{
		case POSTINC:
		case POSTDEC:
		case INC:
		case DEC:
		case POSTINCAT:
		case POSTDECAT:
		case INCAT:
		case DECAT:
		case POSTINCV:
		case POSTDECV:
		case INCV:
		case DECV:
		case POSTINCATV:
		case POSTDECATV:
		case INCATV:
		case DECATV:

		case UNMINUS:

		case LNOT:
		case LOGNOT:

		case POSTINCR:
		case POSTDECR:
		case INCR:
		case DECR:
		case POSTINCATR:
		case POSTDECATR:
		case INCATR:
		case DECATR:
		case POSTINCRV:
		case POSTDECRV:
		case INCRV:
		case DECRV:
		case POSTINCATRV:
		case POSTDECATRV:
		case INCATRV:
		case DECATRV:

		case UNMINUSR:
			return UNARY_OPERATION;


		case REMASS:
		case SHLASS:
		case SHRASS:
		case ANDASS:
		case EXORASS:
		case ORASS:

		case ASS:
		case PLUSASS:
		case MINUSASS:
		case MULTASS:
		case DIVASS:

		case REMASSAT:
		case SHLASSAT:
		case SHRASSAT:
		case ANDASSAT:
		case EXORASSAT:
		case ORASSAT:

		case ASSAT:
		case PLUSASSAT:
		case MINUSASSAT:
		case MULTASSAT:
		case DIVASSAT:

		case REMASSV:
		case SHLASSV:
		case SHRASSV:
		case ANDASSV:
		case EXORASSV:
		case ORASSV:

		case ASSV:
		case PLUSASSV:
		case MINUSASSV:
		case MULTASSV:
		case DIVASSV:

		case REMASSATV:
		case SHLASSATV:
		case SHRASSATV:
		case ANDASSATV:
		case EXORASSATV:
		case ORASSATV:

		case ASSATV:
		case PLUSASSATV:
		case MINUSASSATV:
		case MULTASSATV:
		case DIVASSATV:

		case LREM:
		case LSHL:
		case LSHR:
		case LAND:
		case LEXOR:
		case LOR:
		case LOGAND:
		case LOGOR:

		case EQEQ:
		case NOTEQ:
		case LLT:
		case LGT:
		case LLE:
		case LGE:
		case LPLUS:
		case LMINUS:
		case LMULT:
		case LDIV:

		case ASSR:
		case PLUSASSR:
		case MINUSASSR:
		case MULTASSR:
		case DIVASSR:

		case ASSATR:
		case PLUSASSATR:
		case MINUSASSATR:
		case MULTASSATR:
		case DIVASSATR:

		case ASSRV:
		case PLUSASSRV:
		case MINUSASSRV:
		case MULTASSRV:
		case DIVASSRV:

		case ASSATRV:
		case PLUSASSATRV:
		case MINUSASSATRV:
		case MULTASSATRV:
		case DIVASSATRV:

		case EQEQR:
		case NOTEQR:
		case LLTR:
		case LGTR:
		case LLER:
		case LGER:
		case LPLUSR:
		case LMINUSR:
		case LMULTR:
		case LDIVR:
			return BINARY_OPERATION;


		default:
			return NOT_OPERATION;
	}
}

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

static void assertion_expression(information *const info, node *const nd)
{
	const item_t displ = node_get_arg(nd, 0);
	const item_t assertion_type = node_get_type(nd);

	node_set_next(nd);
	node_set_next(nd); // TIdent

	info->variable_location = LFREE;
	expression(info, nd);

	item_t result = info->answer_reg;

	if (assertion_type != ASS && assertion_type != ASSV)
	{
		// TODO подумать, может стоит выделить load в отдельную функцию tocode_load
		uni_printf(info->io, " %%.%" PRIitem " = load i32, i32* %%var.%" PRIitem ", align 4\n"
			, info->register_num, displ);
		info->register_num++;

		arithmetic_t operation = add_llvm;
		switch (assertion_type)
		{
			case PLUSASS:
			case PLUSASSV:
				operation = add_llvm;
				break;
			case MINUSASS:
			case MINUSASSV:
				operation = sub_llvm;
				break;
			case MULTASS:
			case MULTASSV:
				operation = mul_llvm;
				break;
			case DIVASS:
			case DIVASSV:
				operation = sdiv_llvm;
				break;
			case REMASS:
			case REMASSV:
				operation = srem_llvm;
				break;
			case SHLASS:
			case SHLASSV:
				operation = shl_llvm;
				break;
			case SHRASS:
			case SHRASSV:
				operation = ashr_llvm;
				break;
			case ANDASS:
			case ANDASSV:
				operation = and_llvm;
				break;
			case EXORASS:
			case EXORASSV:
				operation = xor_llvm;
				break;
			case ORASS:
			case ORASSV:
				operation = or_llvm;
				break;
		}

		if (info->answer_type == AREG)
		{
			tocode_arithmetic_reg_reg(info, info->register_num, operation, 
													info->register_num - 1, info->answer_reg);
		}
		else // ACONST
		{
			tocode_arithmetic_reg_const(info, info->register_num, operation, 
													info->register_num - 1, info->answer_const);
		}
			
		result = info->register_num++;
	}

	if (info->answer_type == AREG || (assertion_type != ASS && assertion_type != ASSV))
	{
		// TODO подумать, может стоит выделить store в отдельную функцию tocode_store
		uni_printf(info->io, " store i32 %%.%" PRIitem ", i32* %%var.%" PRIitem ", align 4\n"
			, result, displ);
	}
	else // ACONST && =
	{
		uni_printf(info->io, " store i32 %" PRIitem ", i32* %%var.%" PRIitem ", align 4\n"
			, info->answer_const, displ);
	}
}

static void arithmetic_expression(information *const info, node *const nd)
{
	const item_t operation_type = node_get_type(nd);
	node_set_next(nd);

	info->variable_location = LFREE;
	expression(info, nd);
	answer_t left_type = info->answer_type;
	item_t left_reg = info->answer_reg;
	item_t left_const = info->answer_const;

	info->variable_location = LFREE;
	expression(info, nd);
	answer_t right_type = info->answer_type;
	item_t right_reg = info->answer_reg;
	item_t right_const = info->answer_const;

	arithmetic_t operation = add_llvm;
	switch (operation_type)
	{
		case LPLUS:
		{
			info->answer_const = left_const + right_const;
			operation = add_llvm;
		}
		break;
	}

	if (left_type == AREG && right_type == AREG)
	{
		tocode_arithmetic_reg_reg(info, info->register_num, operation, left_reg, right_reg);
	}
	else if (left_type == AREG && right_type == ACONST)
	{
		tocode_arithmetic_reg_const(info, info->register_num, operation, left_reg, right_const);
	}
	else if (left_type == ACONST && right_type == AREG)
	{
		tocode_arithmetic_const_reg(info, info->register_num, operation, left_const, right_reg);
	}
	else // if (left_type == ACONST && right_type == ACONST)
	{
		info->answer_type = ACONST;
		return;
	}

	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
}

static void unary_operation(information *const info, node *const nd)
{
	node_set_next(nd);
	expression(info, nd);
}

static void binary_operation(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case ASS:
		case ASSV:

		case PLUSASS:
		case PLUSASSV:
		case MINUSASS:
		case MINUSASSV:
		case MULTASS:
		case MULTASSV:
		case DIVASS:
		case DIVASSV:

		case REMASS:
		case REMASSV:
		case SHLASS:
		case SHLASSV:
		case SHRASS:
		case SHRASSV:
		case ANDASS:
		case ANDASSV:
		case EXORASS:
		case EXORASSV:
		case ORASS:
		case ORASSV:
			assertion_expression(info, nd);
			break;
		case LPLUS:
			arithmetic_expression(info, nd);
			break;
		default:
		{
			node_set_next(nd);
			expression(info, nd);
			expression(info, nd);
		}
		break;
	}
}

static void expression(information *const info, node *const nd)
{
	switch (operation_type(nd))
	{
		case UNARY_OPERATION:
			unary_operation(info, nd);
		break;
		case BINARY_OPERATION:
			binary_operation(info, nd);
		break;
		case NOT_OPERATION:
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

	info.arith[add_llvm] = "add";
	info.arith[sub_llvm] = "sub";
	info.arith[mul_llvm] = "mul";
	info.arith[sdiv_llvm] = "sdiv";
	info.arith[srem_llvm] = "srem";
	info.arith[shl_llvm] = "shl";
	info.arith[ashr_llvm] = "ashr";
	info.arith[and_llvm] = "and";
	info.arith[xor_llvm] = "xor";
	info.arith[or_llvm] = "or";

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
				system_error(node_unexpected, node_get_type(&root));
				return -1;
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
	tree_print("new1.txt", &(sx->tree));

	return codegen(io, sx);
}
