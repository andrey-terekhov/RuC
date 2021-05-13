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


#define MAX_ARRAY_DIMENSION		5
#define MAX_ARRAY_DISPL			10000

// TODO: нужно создать enum для типов и передавать его в функции to_code
typedef enum ANSWER
{
	AREG,								/**< Ответ находится в регистре */
	ACONST,								/**< Ответ является константой */
	ALOGIC,								/**< Ответ является логическим значением */
} answer_t;

typedef enum LOCATION
{
	LREG,								/**< Переменная находится в регистре */
	LMEM,								/**< Переменная находится в памяти */
	LFREE,								/**< Свободный запрос значения */
} location_t;

typedef struct array_info
{
	item_t dimention;							/**< Размерность массива */
	// TODO: может лучше enum, а не 1 и 0?
	item_t is_static;							/**< Если массив статический, то 1, иначе 0 */
	item_t borders[MAX_ARRAY_DIMENSION];		/**< Граница массива: константы или номера регистров */
} array_info;

typedef struct information
{
	universal_io *io;							/**< Вывод */
	syntax *sx;									/**< Структура syntax с таблицами */

	item_t string_num;							/**< Номер строки */
	item_t register_num;						/**< Номер регистра */
	item_t label_num;							/**< Номер метки */

	item_t request_reg;							/**< Регистр на запрос */
	location_t variable_location;				/**< Расположение переменной */

	item_t answer_reg;							/**< Регистр с ответом */
	item_t answer_const;						/**< Константа с ответом */
	answer_t answer_type;						/**< Тип ответа */

	item_t label_if;							/**< Метка if */
	item_t label_else;							/**< Метка else */

	array_info current_array_info;				/**< Информация о текущем объявляемом массиве */
	// TODO: хороша ли такая форма хранения информации о массивах?
	// С одной стороны, arrays_info очень разреженный, что плохо
	// С другой стороны, очень удобный доступ по displ, так как оно известно при вырезках 
	array_info arrays_info[MAX_ARRAY_DISPL];	/**< Информация о массивах. Доступ осуществляется по displ */
	// TODO: может лучше enum, а не 1 и 0?
	int was_dynamic;							/**< Если в функции были динамические массивы, то @c 1, иначе @c 0 */
} information;


static void expression(information *const info, node *const nd);
static void block(information *const info, node *const nd);


static void operation_to_io(universal_io *const io, const item_t type)
{
	switch (type)
	{
		case INC:
		case INCV:
		case POSTINC:
		case POSTINCV:
		case PLUSASS:
		case PLUSASSV:
		case LPLUS:
			uni_printf(io, "add nsw");
			break;

		case DEC:
		case DECV:
		case POSTDEC:
		case POSTDECV:
		case MINUSASS:
		case MINUSASSV:
		case LMINUS:
		case UNMINUS:
			uni_printf(io, "sub nsw");
			break;

		case MULTASS:
		case MULTASSV:
		case LMULT:
			uni_printf(io, "mul nsw");
			break;

		case DIVASS:
		case DIVASSV:
		case LDIV:
			uni_printf(io, "sdiv");
			break;

		case REMASS:
		case REMASSV:
		case LREM:
			uni_printf(io, "srem");
			break;

		case SHLASS:
		case SHLASSV:
		case LSHL:
			uni_printf(io, "shl");
			break;

		case SHRASS:
		case SHRASSV:
		case LSHR:
			uni_printf(io, "ashr");
			break;

		case ANDASS:
		case ANDASSV:
		case LAND:
			uni_printf(io, "and");
			break;

		case EXORASS:
		case EXORASSV:
		case LEXOR:
			uni_printf(io, "xor");
			break;

		case ORASS:
		case ORASSV:
		case LOR:
			uni_printf(io, "or");
			break;
		case EQEQ:
			uni_printf(io, "icmp eq");
			break;
		case NOTEQ:
			uni_printf(io, "icmp ne");
			break;
		case LLT:
			uni_printf(io, "icmp slt");
			break;
		case LGT:
			uni_printf(io, "icmp sgt");
			break;
		case LLE:
			uni_printf(io, "icmp sle");
			break;
		case LGE:
			uni_printf(io, "icmp sge");
			break;
	}
}

static void to_code_operation_reg_reg(information *const info, item_t type, item_t fst, item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, type);
	uni_printf(info->io, " i32 %%.%" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_operation_reg_const(information *const info, item_t type, item_t fst, item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, type);
	uni_printf(info->io, " i32 %%.%" PRIitem ", %" PRIitem "\n", fst, snd);
}

static void to_code_operation_const_reg(information *const info, item_t type, item_t fst, item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, type);
	uni_printf(info->io, " i32 %" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static inline void to_code_load(information *const info, item_t result, item_t displ)
{
	uni_printf(info->io, " %%.%" PRIitem " = load i32, i32* %%var.%" PRIitem ", align 4\n", result, displ);
}

static inline void to_code_store_reg(information *const info, item_t reg, item_t displ)
{
	uni_printf(info->io, " store i32 %%.%" PRIitem ", i32* %%var.%" PRIitem ", align 4\n", reg, displ);
}

static inline void to_code_store_const(information *const info, item_t arg, item_t displ)
{
	uni_printf(info->io, " store i32 %" PRIitem ", i32* %%var.%" PRIitem ", align 4\n", arg, displ);
}

static inline void to_code_zext_to(information *const info, item_t arg)
{
	uni_printf(info->io, " %%.%" PRIitem " = zext i1 %%.%" PRIitem " to i32\n", info->register_num, arg);
}

static inline void to_code_label(information *const info, item_t label_num)
{
	uni_printf(info->io, " label%" PRIitem ":\n", label_num);
}

static inline void to_code_unconditional_branch(information *const info, item_t label_num)
{
	uni_printf(info->io, " br label %%label%" PRIitem "\n", label_num);
}

static inline void to_code_conditional_branch(information *const info, item_t reg, item_t label_true, item_t label_false)
{
	uni_printf(info->io, " br i1 %%.%" PRIitem ", label %%label%" PRIitem ", label %%label%" PRIitem "\n", 
		reg, label_true, label_false);
}

static void to_code_alloc_array_static(information *const info, item_t id)
{
	uni_printf(info->io, " %%arr.%" PRIitem " = alloca ", id);

	for (item_t i = 0; i < info->arrays_info[id].dimention; i++)
	{
		uni_printf(info->io, "[%" PRIitem " x ", info->arrays_info[id].borders[i]);
	}
	uni_printf(info->io, "i32");

	for (item_t i = 0; i < info->arrays_info[id].dimention; i++)
	{
		uni_printf(info->io, "]");
	}
	uni_printf(info->io, ", align 4\n");
}

static void to_code_alloc_array_dynamic(information *const info, item_t id)
{
	// выделение памяти на стеке
	item_t to_alloc = info->arrays_info[id].borders[0];

	for (item_t i = 1; i < info->arrays_info[id].dimention; i++)
	{
		uni_printf(info->io, " %%.%" PRIitem " = mul nuw i32 %%.%" PRIitem ", %%.%" PRIitem "\n", 
			info->register_num, to_alloc, info->arrays_info[id].borders[i]);
		to_alloc = info->register_num++;
	}
	uni_printf(info->io, " %%dynarr.%" PRIitem " = alloca i32, i32 %%.%" PRIitem ", align 4\n", id, to_alloc);	
}

static void to_code_stack_save(information *const info)
{
	// команды сохранения состояния стека
	uni_printf(info->io, " %%dyn = alloca i8*, align 4\n");
	uni_printf(info->io, " %%.%" PRIitem " = call i8* @llvm.stacksave()\n", info->register_num);
	uni_printf(info->io, " store i8* %%.%" PRIitem ", i8** %%dyn, align 4\n", info->register_num);
	info->register_num++;
}

static void to_code_stack_load(information *const info)
{
	// команды восстановления состояния стека
	uni_printf(info->io, " %%.%" PRIitem " = load i8*, i8** %%dyn, align 4\n", info->register_num);
	uni_printf(info->io, " call void @llvm.stackrestore(i8* %%.%" PRIitem ")\n", info->register_num);
	info->register_num++;
}


static void check_type_and_branch(information *const info)
{
	switch (info->answer_type)
	{
		case ACONST:
			to_code_unconditional_branch(info, info->answer_const ? info->label_if : info->label_else);
			break;
		case AREG:
		{
			to_code_operation_reg_const(info, NOTEQ, info->answer_reg, 0);
			info->answer_reg = info->register_num++;
		}
		case ALOGIC:
			to_code_conditional_branch(info, info->answer_reg, info->label_if, info->label_else);
			break;
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

			to_code_load(info, info->register_num, displ);
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
				to_code_store_const(info, num, info->request_reg);
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

			const size_t ref_ident = (size_t)node_get_arg(nd, 0);
			const item_t func_type = mode_get(info->sx, (size_t)ident_get_mode(info->sx, ref_ident) + 1);

			node_set_next(nd); // TCall2
			if (func_type == mode_void)
			{
				uni_printf(info->io, " call void @func%zi(", ref_ident);
			}
			else if (func_type == mode_integer)
			{
				uni_printf(info->io, " %%.%" PRIitem " = call i32 @func%zi(", info->register_num, ref_ident);
				info->answer_type = AREG;
				info->answer_reg = info->register_num++;
			}
			// тут будет ещё перечисление аргументов
			uni_printf(info->io, ")\n");
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

static void assignment_expression(information *const info, node *const nd)
{
	const item_t displ = node_get_arg(nd, 0);
	const item_t assignment_type = node_get_type(nd);

	node_set_next(nd);
	node_set_next(nd); // TIdent

	info->variable_location = LFREE;
	expression(info, nd);

	if (info->answer_type == ALOGIC)
	{
		to_code_zext_to(info, info->answer_reg);
		info->answer_type = AREG;
		info->answer_reg = info->register_num++;
	}

	item_t result = info->answer_reg;

	if (assignment_type != ASS && assignment_type != ASSV)
	{
		to_code_load(info, info->register_num, displ);
		info->register_num++;

		if (info->answer_type == AREG)
		{
			to_code_operation_reg_reg(info, assignment_type, info->register_num - 1, info->answer_reg);
		}
		else // ACONST
		{
			to_code_operation_reg_const(info, assignment_type, info->register_num - 1, info->answer_const);
		}
			
		result = info->register_num++;
	}

	if (info->answer_type == AREG || (assignment_type != ASS && assignment_type != ASSV))
	{
		to_code_store_reg(info, result, displ);
	}
	else // ACONST && =
	{
		to_code_store_const(info, info->answer_const, displ);
	}
}

static void arithmetic_expression(information *const info, node *const nd)
{
	const item_t operation_type = node_get_type(nd);
	node_set_next(nd);

	info->variable_location = LFREE;
	expression(info, nd);

	if (info->answer_type == ALOGIC)
	{
		to_code_zext_to(info, info->answer_reg);
		info->answer_type = AREG;
		info->answer_reg = info->register_num++;
	}

	const answer_t left_type = info->answer_type;
	const item_t left_reg = info->answer_reg;
	const item_t left_const = info->answer_const;

	info->variable_location = LFREE;
	expression(info, nd);

	if (info->answer_type == ALOGIC)
	{
		to_code_zext_to(info, info->answer_reg);
		info->answer_type = AREG;
		info->answer_reg = info->register_num++;
	}

	const answer_t right_type = info->answer_type;
	const item_t right_reg = info->answer_reg;
	const item_t right_const = info->answer_const;

	if (left_type == AREG && right_type == AREG)
	{
		to_code_operation_reg_reg(info, operation_type, left_reg, right_reg);
	}
	else if (left_type == AREG && right_type == ACONST)
	{
		to_code_operation_reg_const(info, operation_type, left_reg, right_const);
	}
	else if (left_type == ACONST && right_type == AREG)
	{
		to_code_operation_const_reg(info, operation_type, left_const, right_reg);
	}
	else // if (left_type == ACONST && right_type == ACONST)
	{
		info->answer_type = ACONST;

		switch (operation_type)
		{
			case LPLUS:
				info->answer_const = left_const + right_const;
				break;
			case LMINUS:
				info->answer_const = left_const - right_const;
				break;
			case LMULT:
				info->answer_const = left_const * right_const;
				break;
			case LDIV:
				info->answer_const = left_const / right_const;
				break;
			case LREM:
				info->answer_const = left_const % right_const;
				break;
			case LSHL:
				info->answer_const = left_const << right_const;
				break;
			case LSHR:
				info->answer_const = left_const >> right_const;
				break;
			case LAND:
				info->answer_const = left_const & right_const;
				break;
			case LEXOR:
				info->answer_const = left_const ^ right_const;
				break;
			case LOR:
				info->answer_const = left_const | right_const;
				break;
		}
		return;
	}

	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
}
// TODO: а нужна ли эта функция, очень похожа на arithmetic_expression
static void logic_expression(information *const info, node *const nd)
{
	const item_t operation_type = node_get_type(nd);
	node_set_next(nd);

	info->variable_location = LFREE;
	expression(info, nd);

	if (info->answer_type == ALOGIC)
	{
		to_code_zext_to(info, info->answer_reg);
		info->answer_type = AREG;
		info->answer_reg = info->register_num++;
	}

	const answer_t left_type = info->answer_type;
	const item_t left_reg = info->answer_reg;
	const item_t left_const = info->answer_const;

	info->variable_location = LFREE;
	expression(info, nd);

	if (info->answer_type == ALOGIC)
	{
		to_code_zext_to(info, info->answer_reg);
		info->answer_type = AREG;
		info->answer_reg = info->register_num++;
	}

	const answer_t right_type = info->answer_type;
	const item_t right_reg = info->answer_reg;
	const item_t right_const = info->answer_const;

	if (left_type == AREG && right_type == AREG)
	{
		to_code_operation_reg_reg(info, operation_type, left_reg, right_reg);
	}
	else if (left_type == AREG && right_type == ACONST)
	{
		to_code_operation_reg_const(info, operation_type, left_reg, right_const);
	}
	else if (left_type == ACONST && right_type == AREG)
	{
		to_code_operation_const_reg(info, operation_type, left_const, right_reg);
	}
	else // if (left_type == ACONST && right_type == ACONST)
	{
		info->answer_type = ACONST;

		switch (operation_type)
		{
			case EQEQ:
				info->answer_const = left_const == right_const;
				break;
			case NOTEQ:
				info->answer_const = left_const != right_const;
				break;
			case LLT:
				info->answer_const = left_const < right_const;
				break;
			case LGT:
				info->answer_const = left_const > right_const;
				break;
			case LLE:
				info->answer_const = left_const <= right_const;
				break;
			case LGE:
				info->answer_const = left_const >= right_const;
				break;
		}
		return;
	}

	info->answer_type = ALOGIC;
	info->answer_reg = info->register_num++;
}

// Обрабатываются операции инкремента/декремента и постинкремента/постдекремента
static void inc_dec_expression(information *const info, node *const nd)
{
	const item_t displ = node_get_arg(nd, 0);
	const item_t operation_type = node_get_type(nd);

	node_set_next(nd);
	node_set_next(nd); // TIdent

	to_code_load(info, info->register_num, displ);
	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
	
	switch (operation_type)
	{
		case INC:
		case INCV:
		case DEC:
		case DECV:
			info->answer_reg = info->register_num;
		case POSTINC:
		case POSTINCV:
		case POSTDEC:
		case POSTDECV:
			to_code_operation_reg_const(info, operation_type, info->register_num - 1, 1);
			break;
	}

	to_code_store_reg(info, info->register_num, displ);
	info->register_num++;
}

static void unary_operation(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case POSTINC:
		case POSTINCV:
		case POSTDEC:
		case POSTDECV:
		case INC:
		case INCV:
		case DEC:
		case DECV:
			inc_dec_expression(info, nd);
			break;
		case UNMINUS:
		{
			node_set_next(nd);

			info->variable_location = LREG;
			expression(info, nd);

			if (info->answer_type == ALOGIC)
			{
				to_code_zext_to(info, info->answer_reg);
				info->answer_reg = info->register_num++;
			}

			to_code_operation_const_reg(info, UNMINUS, 0, info->answer_reg);
			info->answer_type = AREG;
			info->answer_reg = info->register_num++;
		}
		break;
		case LOGNOT:
		{
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;

			node_set_next(nd);
			info->label_if = old_label_else;
			info->label_else = old_label_if;
			expression(info, nd);
		}
			break;
		// TODO: забыл сделать битовое отрицание, нужно доделать
		default:
		{
			node_set_next(nd);
			expression(info, nd);
		}
		break;
	}
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
			assignment_expression(info, nd);
			break;


		case LPLUS:
		case LMINUS:
		case LMULT:
		case LDIV:

		case LREM:
		case LSHL:
		case LSHR:
		case LAND:
		case LEXOR:
		case LOR:
			arithmetic_expression(info, nd);
			break;


		case EQEQ:
		case NOTEQ:
		case LLT:
		case LGT:
		case LLE:
		case LGE:
			logic_expression(info, nd);
			break;

		// TODO: протестировать и при необходимости реализовать случай, когда && и || есть в арифметических выражениях
		case LOGOR:
		case LOGAND:
		{
			const item_t label_next = info->label_num++;
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;

			if (node_get_type(nd) == LOGOR)
			{
				info->label_else = label_next;
			}
			else // (node_get_type(nd) == LOGAND)
			{
				info->label_if = label_next;
			}
			
			node_set_next(nd);
			expression(info, nd);

			// TODO: сделать обработку других ответов
			// постараться использовать функцию check_type_and_branch
			if (info->answer_type == ALOGIC)
			{
				to_code_conditional_branch(info, info->answer_reg, info->label_if, info->label_else);
			}

			to_code_label(info, label_next);
			info->label_if = old_label_if;
			info->label_else = old_label_else;

			expression(info, nd);
		}
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
			unary_operation(info, nd);
			break;


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
			binary_operation(info, nd);
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
			block(info, nd);
			node_set_next(nd); // TEnd
		}
		break;
		case TIf:
		{
			const item_t ref_else = node_get_arg(nd, 0);
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;
			const item_t label_if = info->label_num++;
			const item_t label_else = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_if = label_if;
			info->label_else = label_else;

			node_set_next(nd);

			info->variable_location = LFREE;
			expression(info, nd);

			check_type_and_branch(info);
			
			to_code_label(info, label_if);

			statement(info, nd);
			to_code_unconditional_branch(info, label_end);
			to_code_label(info, label_else);
			if (ref_else)
			{
				statement(info, nd);
			}

			to_code_unconditional_branch(info, label_end);
			to_code_label(info, label_end);

			info->label_if = old_label_if;
			info->label_else = old_label_else;
		}
		break;
		case TSwitch:
		case TCase:
		case TDefault:
		{
			node_set_next(nd);
			expression(info, nd);
			statement(info, nd);
		}
		break;
		case TWhile:
		{
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_if = label_body;
			info->label_else = label_end;

			node_set_next(nd);

			to_code_unconditional_branch(info, label_condition);
			to_code_label(info, label_condition);

			info->variable_location = LFREE;
			expression(info, nd);

			check_type_and_branch(info);

			to_code_label(info, label_body);

			statement(info, nd);

			to_code_unconditional_branch(info, label_condition);
			to_code_label(info, label_end);

			info->label_if = old_label_if;
			info->label_else = old_label_else;
		}
		break;
		case TDo:
		{
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;
			const item_t label_loop = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_if = label_loop;
			info->label_else = label_end;

			node_set_next(nd);

			to_code_unconditional_branch(info, label_loop);
			to_code_label(info, label_loop);

			statement(info, nd);

			info->variable_location = LFREE;
			expression(info, nd);

			check_type_and_branch(info);

			to_code_label(info, label_end);

			info->label_if = old_label_if;
			info->label_else = old_label_else;
		}
		break;
		// TODO: проверялось, только если в for присутствуют все блоки: инициализация, условие, модификация
		// нужно проверить и реализовать случаи, когда какие-нибудь из этих блоков отсутсвуют
		case TFor:
		{
			const item_t ref_from = node_get_arg(nd, 0);
			const item_t ref_cond = node_get_arg(nd, 1);
			const item_t ref_incr = node_get_arg(nd, 2);
			const item_t old_label_if = info->label_if;
			const item_t old_label_else = info->label_else;
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_incr = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_if = label_body;
			info->label_else = label_end;

			node_set_next(nd);

			if (ref_from)
			{
				expression(info, nd);
			}

			to_code_unconditional_branch(info, label_condition);
			to_code_label(info, label_condition);

			if (ref_cond)
			{
				expression(info, nd);
			}
			// TODO: проверить разные типы условий: const, reg
			check_type_and_branch(info);
			to_code_label(info, label_incr);

			if (ref_incr)
			{
				expression(info, nd);
			}
			to_code_unconditional_branch(info, label_condition);

			to_code_label(info, label_body);

			statement(info, nd);
			to_code_unconditional_branch(info, label_incr);
			to_code_label(info, label_end);

			info->label_if = old_label_if;
			info->label_else = old_label_else;
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
		case TGoto:
			node_set_next(nd);
			break;
		case TReturnvoid:
		{
			if (info->was_dynamic == 1)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			uni_printf(info->io, " ret void\n");
		}
		break;
		case TReturnval:
		{
			if (info->was_dynamic == 1)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			info->variable_location = LREG;
			expression(info, nd);
			// TODO: добавить обработку других ответов (ALOGIC)
			if (info->answer_type == ACONST)
			{
				uni_printf(info->io, " ret i32 %" PRIitem "\n", info->answer_const);
			}
			else if (info->answer_type == AREG)
			{
				uni_printf(info->io, " ret i32 %%.%" PRIitem "\n", info->answer_reg);
			}
			node_set_next(nd); // TReturnvoid
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
	node_set_next(nd); // TBegin
	while (node_get_type(nd) != TEnd)
	{
		switch (node_get_type(nd))
		{
			case TDeclarr:
			{
				const item_t N = node_get_arg(nd, 0);

				node_set_next(nd);
				info->current_array_info.dimention = N;
				info->current_array_info.is_static = 1;
				for (item_t i = 0; i < N; i++)
				{
					info->variable_location = LFREE;
					expression(info, nd);

					// TODO: а если часть границ статическая, а часть динамическая?
					// наверное, в таком случае границы надо хранить в раздельных массивах
					// но об этом потом
					if (info->answer_type == ACONST)
					{
						info->current_array_info.borders[i] = info->answer_const;
					}
					else // if (info->answer_type == ARER) динамический массив
					{
						info->current_array_info.borders[i] = info->answer_reg;
						info->current_array_info.is_static = 0;
					}			
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
					// TODO: может switch сделать, когда больше типов добавляться будет
					if (elem_type == mode_integer)
					{
						uni_printf(info->io, " %%var.%" PRIitem " = alloca i32, align 4\n", displ);
					}
					else if (elem_type == mode_float)
					{
						uni_printf(info->io, " %%var.%" PRIitem " = alloca double, align 4\n", displ);
					}
					info->variable_location = LMEM;
					info->request_reg = displ;
				}
				else // массивы
				{
					info->arrays_info[displ] = info->current_array_info;
					if (info->current_array_info.is_static == 1)
					{
						to_code_alloc_array_static(info, displ);
					}
					else
					{
						// TODO: нужно ещё сделать восстановление стека после выделения памяти
						if (info->was_dynamic == 0)
						{
							to_code_stack_save(info);
						}
						to_code_alloc_array_dynamic(info, displ);
						info->was_dynamic = 1;
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
}

static int codegen(universal_io *const io, syntax *const sx)
{
	item_t to_declare_stack_functions = 0;
	information info;
	info.io = io;
	info.sx = sx;
	info.string_num = 1;
	info.register_num = 1;
	info.label_num = 1;
	info.variable_location = LREG;
	info.request_reg = 0;
	info.answer_reg = 0;
	info.was_dynamic = 0;

	node root = node_get_root(&sx->tree);
	while (node_set_next(&root) == 0)
	{
		switch (node_get_type(&root))
		{
			case TFuncdef:
			{
				const size_t ref_ident = (size_t)node_get_arg(&root, 0);
				const item_t func_type = mode_get(info.sx, (size_t)ident_get_mode(info.sx, ref_ident) + 1);

				if (ident_get_prev(info.sx, ref_ident) == LMAIN)
				{
					uni_printf(info.io, "define i32 @main(");
				}
				else if (func_type == mode_void)
				{
					uni_printf(info.io, "define void @func%zi(", ref_ident);
				}
				else if (func_type == mode_integer)
				{
					uni_printf(info.io, "define i32 @func%zi(", ref_ident);
				}
				uni_printf(info.io, ") {\n");

				node_set_next(&root);
				block(&info, &root);
				uni_printf(info.io, "}\n\n");

				if (info.was_dynamic == 1)
				{
					to_declare_stack_functions = 1;
				}
				info.was_dynamic = 0;
			}
			break;
			case TEnd:
				break;
			default:
				system_error(node_unexpected, node_get_type(&root));
				return -1;
		}
	}

	if (to_declare_stack_functions == 1)
	{
		uni_printf(info.io, "declare i8* @llvm.stacksave()\n");
		uni_printf(info.io, "declare void @llvm.stackrestore(i8*)\n");
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
