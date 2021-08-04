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
#include <string.h>
#include "codes.h"
#include "errors.h"
#include "hash.h"
#include "llvmopt.h"
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"


const size_t HASH_TABLE_SIZE = 1024;
const size_t IS_STATIC = 0;


typedef enum ANSWER
{
	AREG,								/**< Ответ находится в регистре */
	ACONST,								/**< Ответ является константой */
	ALOGIC,								/**< Ответ является логическим значением */
	AMEM,								/**< Ответ находится в памяти */
} answer_t;

typedef enum LOCATION
{
	LREG,								/**< Переменная находится в регистре */
	LMEM,								/**< Переменная находится в памяти */
	LFREE,								/**< Свободный запрос значения */
} location_t;

typedef struct information
{
	syntax *sx;							/**< Структура syntax с таблицами */

	item_t string_num;					/**< Номер строки */
	item_t register_num;				/**< Номер регистра */
	item_t label_num;					/**< Номер метки */
	item_t init_num;					/**< Счётчик для инициализации */

	item_t request_reg;					/**< Регистр на запрос */
	location_t variable_location;		/**< Расположение переменной */

	item_t answer_reg;					/**< Регистр с ответом */
	item_t answer_const;				/**< Константа с ответом */
	double answer_const_double;			/**< Константа с ответом типа double */
	answer_t answer_type;				/**< Тип ответа */
	item_t answer_value_type;			/**< Тип значения */

	item_t label_true;					/**< Метка перехода при true */
	item_t label_false;					/**< Метка перехода при false */
	item_t label_break;					/**< Метка перехода для break */
	item_t label_continue;				/**< Метка перехода для continue */

	hash arrays;						/**< Хеш таблица с информацией о массивах:
												@с key		 - смещение массива
												@c value[0]	 - флаг статичности
												@c value[1..MAX] - границы массива */
	int was_dynamic;					/**< Истина, если в функции были динамические массивы */
	int was_memcpy;						/**< Истина, если memcpy использовалась для инициализации */
	// костыль, жду исправлений в дереве
	int was_return;
} information;


static void expression(information *const info, node *const nd);
static void block(information *const info, node *const nd);


static double to_double(const int64_t fst, const int64_t snd)
{
	int64_t num = (snd << 32) | (fst & 0x00000000ffffffff);
	double numdouble;
	memcpy(&numdouble, &num, sizeof(double));

	return numdouble;
}

static void type_to_io(information *const info, const item_t type)
{
	if (type_is_integer(type))
	{
		uni_printf(info->sx->io, "i32");
	}
	else if (type_is_floating(type))
	{
		uni_printf(info->sx->io, "double");
	}
	else if (type_is_void(type))
	{
		uni_printf(info->sx->io, "void");
	}
	else if (type_is_structure(info->sx, type))
	{
		uni_printf(info->sx->io, "%%struct_opt.%" PRIitem, type);
	}
	else if (type_is_pointer(info->sx, type))
	{
		type_to_io(info, type_get(info->sx, (size_t)type + 1));
		uni_printf(info->sx->io, "*");
	}
}

static void operation_to_io(universal_io *const io, const item_t type)
{
	switch (type)
	{
		case UN_PREINC:
		case UN_POSTINC:
		case BIN_ADD_ASSIGN:
		case BIN_ADD:
			uni_printf(io, "add nsw");
			break;

		case UN_PREDEC:
		case UN_POSTDEC:
		case BIN_SUB_ASSIGN:
		case BIN_SUB:
		case UN_MINUS:
			uni_printf(io, "sub nsw");
			break;

		case BIN_MUL_ASSIGN:
		case BIN_MUL:
			uni_printf(io, "mul nsw");
			break;

		case BIN_DIV_ASSIGN:
		case BIN_DIV:
			uni_printf(io, "sdiv");
			break;

		case BIN_REM_ASSIGN:
		case BIN_REM:
			uni_printf(io, "srem");
			break;

		case BIN_SHL_ASSIGN:
		case BIN_SHL:
			uni_printf(io, "shl");
			break;

		// case OP_SHR_ASSIGN:
		// case OP_SHR_ASSIGN_V:
		// case OP_SHR:
		// case OP_SHR_ASSIGN_AT:
		// case OP_SHR_ASSIGN_AT_V:
		// 	uni_printf(io, "ashr");
		// 	break;

		// case OP_AND_ASSIGN:
		// case OP_AND_ASSIGN_V:
		// case OP_AND:
		// case OP_AND_ASSIGN_AT:
		// case OP_AND_ASSIGN_AT_V:
		// 	uni_printf(io, "and");
		// 	break;

		// case OP_XOR_ASSIGN:
		// case OP_XOR_ASSIGN_V:
		// case OP_XOR:
		// case OP_NOT:
		// case OP_XOR_ASSIGN_AT:
		// case OP_XOR_ASSIGN_AT_V:
		// 	uni_printf(io, "xor");
		// 	break;

		// case OP_OR_ASSIGN:
		// case OP_OR_ASSIGN_V:
		// case OP_OR:
		// case OP_OR_ASSIGN_AT:
		// case OP_OR_ASSIGN_AT_V:
		// 	uni_printf(io, "or");
		// 	break;

		// case OP_EQ:
		// 	uni_printf(io, "icmp eq");
		// 	break;
		// case OP_NE:
		// 	uni_printf(io, "icmp ne");
		// 	break;
		// case OP_LT:
		// 	uni_printf(io, "icmp slt");
		// 	break;
		// case OP_GT:
		// 	uni_printf(io, "icmp sgt");
		// 	break;
		// case OP_LE:
		// 	uni_printf(io, "icmp sle");
		// 	break;
		// case OP_GE:
		// 	uni_printf(io, "icmp sge");
		// 	break;

		// case OP_PRE_INC_R:
		// case OP_PRE_INC_R_V:
		// case OP_POST_INC_R:
		// case OP_POST_INC_R_V:
		// case OP_ADD_ASSIGN_R:
		// case OP_ADD_ASSIGN_R_V:
		// case OP_ADD_R:
		// case OP_ADD_ASSIGN_AT_R:
		// case OP_ADD_ASSIGN_AT_R_V:
		// case OP_PRE_INC_AT_R:
		// case OP_PRE_INC_AT_R_V:
		// case OP_POST_INC_AT_R:
		// case OP_POST_INC_AT_R_V:
		// 	uni_printf(io, "fadd");
		// 	break;

		// case OP_PRE_DEC_R:
		// case OP_PRE_DEC_R_V:
		// case OP_POST_DEC_R:
		// case OP_POST_DEC_R_V:
		// case OP_SUB_ASSIGN_R:
		// case OP_SUB_ASSIGN_R_V:
		// case OP_SUB_R:
		// case OP_UNMINUS_R:
		// case OP_SUB_ASSIGN_AT_R:
		// case OP_SUB_ASSIGN_AT_R_V:
		// case OP_PRE_DEC_AT_R:
		// case OP_PRE_DEC_AT_R_V:
		// case OP_POST_DEC_AT_R:
		// case OP_POST_DEC_AT_R_V:
		// 	uni_printf(io, "fsub");
		// 	break;

		// case OP_MUL_ASSIGN_R:
		// case OP_MUL_ASSIGN_R_V:
		// case OP_MUL_R:
		// case OP_MUL_ASSIGN_AT_R:
		// case OP_MUL_ASSIGN_AT_R_V:
		// 	uni_printf(io, "fmul");
		// 	break;

		// case OP_DIV_ASSIGN_R:
		// case OP_DIV_ASSIGN_R_V:
		// case OP_DIV_R:
		// case OP_DIV_ASSIGN_AT_R:
		// case OP_DIV_ASSIGN_AT_R_V:
		// 	uni_printf(io, "fdiv");
		// 	break;

		// case OP_EQ_R:
		// 	uni_printf(io, "fcmp oeq");
		// 	break;
		// case OP_NE_R:
		// 	uni_printf(io, "fcmp one");
		// 	break;
		// case OP_LT_R:
		// 	uni_printf(io, "fcmp olt");
		// 	break;
		// case OP_GT_R:
		// 	uni_printf(io, "fcmp ogt");
		// 	break;
		// case OP_LE_R:
		// 	uni_printf(io, "fcmp ole");
		// 	break;
		// case OP_GE_R:
		// 	uni_printf(io, "fcmp oge");
		// 	break;
	}
}

static void to_code_operation_reg_reg(information *const info, const item_t operation
	, const item_t fst, const item_t snd, const item_t type)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation);
	uni_printf(info->sx->io, " ");
	type_to_io(info, type);
	uni_printf(info->sx->io, " %%.%" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_load(information *const info, const item_t result, const item_t displ, const item_t type
	, const int is_array, const int is_pointer)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = load ", result);
	type_to_io(info, type);
	uni_printf(info->sx->io, "%s, ", is_pointer ? "*" : "");
	type_to_io(info, type);
	uni_printf(info->sx->io, "*%s %%%s.%" PRIitem ", align 4\n", is_pointer ? "*" : "", is_array ? "" : "var", displ);
}

static inline void to_code_store_reg(information *const info, const item_t reg, const item_t displ, const item_t type
	, const int is_array, const int is_pointer)
{
	uni_printf(info->sx->io, " store ");
	type_to_io(info, type);
	uni_printf(info->sx->io, "%s %%%s.%" PRIitem ", ", is_pointer ? "*" : "", is_pointer ? "var" : "", reg);
	type_to_io(info, type);
	uni_printf(info->sx->io, "*%s %%%s.%" PRIitem ", align 4\n", is_pointer ? "*" : "", is_array ? "" : "var", displ);
}

static inline void to_code_store_const_i32(information *const info, const item_t arg, const item_t displ
	, const int is_array)
{
	uni_printf(info->sx->io, " store i32 %" PRIitem ", i32* %%%s.%" PRIitem ", align 4\n"
		, arg, is_array ? "" : "var", displ);
}

static inline void to_code_store_const_double(information *const info, const double arg, const item_t displ
	, const int is_array)
{
	uni_printf(info->sx->io, " store double %f, double* %%%s.%" PRIitem ", align 4\n", arg, is_array ? "" : "var", displ);
}

static void to_code_try_zext_to(information *const info)
{
	if (info->answer_type != ALOGIC)
	{
		return;
	}

	uni_printf(info->sx->io, " %%.%" PRIitem " = zext i1 %%.%" PRIitem " to i32\n", info->register_num, info->answer_reg);
	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
}

static inline void to_code_label(information *const info, const item_t label_num)
{
	uni_printf(info->sx->io, " label%" PRIitem ":\n", label_num);
}

static inline void to_code_unconditional_branch(information *const info, const item_t label_num)
{
	uni_printf(info->sx->io, " br label %%label%" PRIitem "\n", label_num);
}

static inline void to_code_conditional_branch(information *const info)
{
	uni_printf(info->sx->io, " br i1 %%.%" PRIitem ", label %%label%" PRIitem ", label %%label%" PRIitem "\n"
		, info->answer_reg, info->label_true, info->label_false);
}

static void to_code_stack_load(information *const info)
{
	// команды восстановления состояния стека
	uni_printf(info->sx->io, " %%.%" PRIitem " = load i8*, i8** %%dyn, align 4\n", info->register_num);
	uni_printf(info->sx->io, " call void @llvm.stackrestore(i8* %%.%" PRIitem ")\n", info->register_num);
	info->register_num++;
}

static void to_code_alloc_array_static(information *const info, const size_t index, const item_t type)
{
	uni_printf(info->sx->io, " %%arr.%" PRIitem " = alloca ", hash_get_key(&info->arrays, index));

	const size_t dim = hash_get_amount_by_index(&info->arrays, index) - 1;
	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "[%" PRIitem " x ", hash_get_by_index(&info->arrays, index, i));
	}
	type_to_io(info, type);

	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "]");
	}
	uni_printf(info->sx->io, ", align 4\n");
}

static void to_code_init_array(information *const info, const size_t index, const item_t type)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = bitcast ", info->register_num);
	info->register_num++;

	const size_t dim = hash_get_amount_by_index(&info->arrays, index) - 1;
	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "[%" PRIitem " x ", hash_get_by_index(&info->arrays, index, i));
	}
	type_to_io(info, type);

	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "]");
	}
	uni_printf(info->sx->io, "* %%arr.%" PRIitem " to i8*\n", hash_get_key(&info->arrays, index));

	uni_printf(info->sx->io, " call void @llvm.memcpy.p0i8.p0i8.i32(i8* %%.%" PRIitem ", i8* bitcast (", info->register_num - 1);
	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "[%" PRIitem " x ", hash_get_by_index(&info->arrays, index, i));
	}
	type_to_io(info, type);

	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->sx->io, "]");
	}
	
	uni_printf(info->sx->io, "* @arr_init.%" PRIitem " to i8*), i32 %" PRIitem ", i32 %i, i1 false)\n"
		, info->init_num
		, (type_is_floating(type) ? 8 : 4) * hash_get_by_index(&info->arrays, index, 1)
		, type_is_floating(type) ? 8 : 4);

	info->init_num++;
	info->was_memcpy = 1;
}

static void check_type_and_branch(information *const info)
{
	switch (info->answer_type)
	{
		case ACONST:
			to_code_unconditional_branch(info, info->answer_const ? info->label_true : info->label_false);
			break;
		case AREG:
		{
			// to_code_operation_reg_const_i32(info, BIN_NE, info->answer_reg, 0);
			info->answer_reg = info->register_num++;
		}
		case ALOGIC:
			to_code_conditional_branch(info);
			break;
		case AMEM:
			break;
	}
}

static void operand(information *const info, node *const nd)
{
	if (node_get_type(nd) == OP_NOP)
	{
		node_set_next(nd);
	}

	switch (node_get_type(nd))
	{
		// case OP_IDENT:
		case OP_SELECT:
			node_set_next(nd);
			break;
		// case OP_IDENT_TO_ADDR:
		// {
		// 	info->answer_reg = node_get_arg(nd, 0);
		// 	info->answer_type = AMEM;
		// 	node_set_next(nd);
		// }
		// break;
		case OP_IDENTIFIER:
		{
			const item_t type = node_get_arg(nd, 0);
			const item_t displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
			int is_addr_to_val = 0;

			node_set_next(nd);
			// if (node_get_type(nd) == OP_ADDR_TO_VAL)
			// {
			// 	to_code_load(info, info->register_num, displ, mode_integer, 0, 1);
			// 	info->register_num++;
			// 	info->variable_location = LREG;
			// 	node_set_next(nd);
			// 	is_addr_to_val = 1;
			// }
			to_code_load(info, info->register_num, is_addr_to_val ? info->register_num - 1 : displ, type
				, is_addr_to_val, info->variable_location == LMEM ? 1 : 0);
			info->answer_reg = info->register_num++;
			info->answer_type = AREG;
			info->answer_value_type = type;
		}
		break;
		case OP_CONSTANT:
		{
			const item_t type = node_get_arg(nd, 0);

			if (type_is_integer(type))
			{
				const item_t num = node_get_arg(nd, 2);

				if (info->variable_location == LMEM)
				{
					to_code_store_const_i32(info, num, info->request_reg, 0);
					info->answer_type = AREG;
				}
				else
				{
					info->answer_type = ACONST;
					info->answer_const = num;
					info->answer_value_type = TYPE_INTEGER;
				}
			}
			else
			{
				const double num = to_double(node_get_arg(nd, 2), node_get_arg(nd, 3));

				if (info->variable_location == LMEM)
				{
					to_code_store_const_double(info, num, info->request_reg, 0);
					info->answer_type = AREG;
				}
				else
				{
					info->answer_type = ACONST;
					info->answer_const_double = num;
					info->answer_value_type = TYPE_FLOATING;
				}
			}

			node_set_next(nd);
		}
		break;
		// case OP_STRING:
		// 	node_set_next(nd);
		// 	break;
		// case OP_SLICE_IDENT:
		// {
		// 	const item_t displ = node_get_arg(nd, 0);
		// 	// TODO: как и в llvmopt, это работает только для двумерных массивов,
		// 	// 	надо подумать над этим потом (может общую функцию сделать?)
		// 	const item_t type = node_get_arg(nd, 1) > 0
		// 							? mode_get(info->sx, (size_t)node_get_arg(nd, 1) + 1)
		// 							: node_get_arg(nd, 1);
		// 	item_t cur_dimension = hash_get_amount(&info->arrays, displ) - 2;
		// 	const location_t location = info->variable_location;
		// 	node_set_next(nd);

		// 	info->variable_location = LFREE;
		// 	expression(info, nd);

		// 	// TODO: пока только для динамических массивов размерности 2
		// 	if (!hash_get(&info->arrays, displ, IS_STATIC) && cur_dimension == 1)
		// 	{
		// 		if (info->answer_type == ACONST)
		// 		{
		// 			to_code_operation_const_reg_i32(info, OP_MUL, info->answer_const, hash_get(&info->arrays, displ, 2));
		// 		}
		// 		else // if (info->answer_type == AREG)
		// 		{
		// 			to_code_operation_reg_reg(info, OP_MUL, info->answer_reg, hash_get(&info->arrays, displ, 2),
		// 				mode_integer);
		// 		}

		// 		info->answer_type = AREG;
		// 		info->answer_reg = info->register_num++;
		// 	}

		// 	// Проверка, что значение cur_dimension корректное и в пределах допустимого
		// 	// cur_dimension не определена пока что для массивов в структурах и массивов-аргументов функций
		// 	if (-1 < cur_dimension && cur_dimension < 5)
		// 	{
		// 		to_code_slice(info, displ, cur_dimension, 0, type);
		// 	}

		// 	item_t prev_slice = info->register_num - 1;
		// 	while (node_get_type(nd) == OP_SLICE)
		// 	{
		// 		node_set_next(nd);
		// 		info->variable_location = LFREE;
		// 		expression(info, nd);
		// 		cur_dimension--;

		// 		// Проверка, что значение cur_dimension корректное и в пределах допустимого
		// 		// cur_dimension не определена пока что для массивов в структурах и массивов-аргументов функций
		// 		if (-1 < cur_dimension && cur_dimension < 5)
		// 		{
		// 			to_code_slice(info, displ, cur_dimension, prev_slice, type);
		// 		}
		// 		prev_slice = info->register_num - 1;
		// 	}

		// 	// TODO: может это замена LMEM? Подумать, когда будут реализовываться указатели
		// 	if (node_get_type(nd) == OP_ADDR_TO_VAL)
		// 	{
		// 		node_set_next(nd);
		// 	}

		// 	if (location != LMEM)
		// 	{
		// 		to_code_load(info, info->register_num, info->register_num - 1, type, 1, 0);
		// 		info->register_num++;
		// 	}

		// 	info->answer_reg = info->register_num - 1;
		// 	info->answer_type = AREG;
		// 	info->answer_value_type = type;
		// }
		// break;
		// case OP_CALL1:
		// {
		// 	const item_t args = node_get_arg(nd, 0);
		// 	item_t arguments[128];
		// 	double arguments_double[128];
		// 	answer_t arguments_type[128];
		// 	item_t arguments_value_type[128];

		// 	node_set_next(nd);
		// 	node_set_next(nd); // OP_IDENT
		// 	for (item_t i = 0; i < args; i++)
		// 	{
		// 		info->variable_location = LFREE;
		// 		expression(info, nd);
		// 		// TODO: сделать параметры других типов (логическое)
		// 		arguments_type[i] = info->answer_type;
		// 		arguments_value_type[i] = info->answer_value_type;
		// 		if (info->answer_type == AREG)
		// 		{
		// 			arguments[i] = info->answer_reg;
		// 		}
		// 		else if (mode_is_int(info->answer_value_type)) // ACONST
		// 		{
		// 			arguments[i] = info->answer_const;
		// 		}
		// 		else // double
		// 		{
		// 			arguments_double[i] = info->answer_const_double;
		// 		}
		// 	}

		// 	const size_t ref_ident = (size_t)node_get_arg(nd, 0);
		// 	const item_t func_type = mode_get(info->sx, (size_t)ident_get_mode(info->sx, ref_ident) + 1);

		// 	node_set_next(nd); // OP_CALL2

		// 	if (func_type != mode_void)
		// 	{
		// 		uni_printf(info->sx->io, " %%.%" PRIitem " =", info->register_num);
		// 		info->answer_type = AREG;
		// 		info->answer_value_type = func_type;
		// 		info->answer_reg = info->register_num++;
		// 	}
		// 	uni_printf(info->sx->io, " call ");
		// 	type_to_io(info, func_type);
		// 	uni_printf(info->sx->io, " @func%zi(", ref_ident);

		// 	for (item_t i = 0; i < args; i++)
		// 	{
		// 		if (i != 0)
		// 		{
		// 			uni_printf(info->sx->io, ", ");
		// 		}

		// 		type_to_io(info, arguments_value_type[i]);
		// 		uni_printf(info->sx->io, " signext ");
		// 		if (arguments_type[i] == AREG)
		// 		{
		// 			uni_printf(info->sx->io, "%%.%" PRIitem, arguments[i]);
		// 		}
		// 		else if (mode_is_int(arguments_value_type[i])) // ACONST
		// 		{
		// 			uni_printf(info->sx->io, "%" PRIitem, arguments[i]);
		// 		}
		// 		else // double
		// 		{
		// 			uni_printf(info->sx->io, "%f", arguments_double[i]);
		// 		}
		// 	}
		// 	uni_printf(info->sx->io, ")\n");
		// }
		// break;
		default:
			node_set_next(nd);
			break;
	}
}

static void assignment_expression(information *const info, node *const nd)
{
	const item_t assignment_type = node_get_arg(nd, 2);
	const item_t operation_type = node_get_arg(nd, 0);
	item_t displ = 0, memory_reg = 0;
	int is_array;

	node_set_next(nd);
	if (node_get_type(nd) == OP_IDENTIFIER)
	{
		is_array = 0;
		displ = ident_get_displ(info->sx, node_get_arg(nd, 2));
		node_set_next(nd);
	}
	else // OP_SLICE_IDENT
	{
		is_array = 1;
		info->variable_location = LMEM;
		operand(info, nd); // OP_SLICE_IDENT
		memory_reg = info->answer_reg;
	}

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_zext_to(info);
	item_t result = info->answer_reg;

	if (assignment_type != BIN_ASSIGN)
	{
		to_code_load(info, info->register_num, is_array ? memory_reg : displ, operation_type, is_array, 0);
		info->register_num++;

		if (info->answer_type == AREG)
		{
			to_code_operation_reg_reg(info, assignment_type, info->register_num - 1, info->answer_reg, operation_type);
		}
		// ACONST
		else if (type_is_integer(operation_type))
		{
			// to_code_operation_reg_const_i32(info, assignment_type, info->register_num - 1, info->answer_const);
		}
		else
		{
			// to_code_operation_reg_const_double(info, assignment_type, info->register_num - 1
			// 	, info->answer_const_double);
		}

		result = info->register_num++;
		info->answer_type = AREG;
	}

	if (info->answer_type == AREG || info->answer_type == AMEM)
	{
		to_code_store_reg(info, result, is_array ? memory_reg : displ, operation_type, is_array
			, info->answer_type == AMEM ? 1 : 0);
	}
	// ACONST && =
	else if (type_is_integer(operation_type))
	{
		to_code_store_const_i32(info, info->answer_const, is_array ? memory_reg : displ, is_array);
	}
	else
	{
		to_code_store_const_double(info, info->answer_const_double, is_array ? memory_reg : displ, is_array);
	}
}

static void binary_operation(information *const info, node *const nd)
{
	if (operation_is_assignment(node_get_arg(nd, 2)))
	{
		assignment_expression(info, nd);
		return;
	}
}

static void expression(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_UNARY:
			// unary_operation(info, nd);
			break;
		case OP_BINARY:
			binary_operation(info, nd);
			break;
		default:
			operand(info, nd);
			break;
	}
}

static void statement(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_BLOCK:
			block(info, nd);
			break;
		case OP_IF:
		{
			const item_t ref_else = node_get_arg(nd, 0);
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;
			const item_t label_if = info->label_num++;
			const item_t label_else = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_if;
			info->label_false = label_else;

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

			info->label_true = old_label_true;
			info->label_false = old_label_false;
		}
		break;
		case OP_SWITCH:
		case OP_CASE:
		case OP_DEFAULT:
		{
			node_set_next(nd);
			expression(info, nd);
			statement(info, nd);
		}
		break;
		case OP_WHILE:
		{
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;
			const item_t old_label_break = info->label_break;
			const item_t old_label_continue = info->label_continue;
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_body;
			info->label_false = label_end;
			info->label_break = label_end;
			info->label_continue = label_body;

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

			info->label_true = old_label_true;
			info->label_false = old_label_false;
			info->label_break = old_label_break;
			info->label_continue = old_label_continue;
		}
		break;
		case OP_DO:
		{
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;
			const item_t old_label_break = info->label_break;
			const item_t old_label_continue = info->label_continue;
			const item_t label_loop = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_loop;
			info->label_false = label_end;
			info->label_break = label_end;
			info->label_continue = label_loop;

			node_set_next(nd);
			to_code_unconditional_branch(info, label_loop);
			to_code_label(info, label_loop);
			statement(info, nd);

			info->variable_location = LFREE;
			expression(info, nd);

			check_type_and_branch(info);

			to_code_label(info, label_end);

			info->label_true = old_label_true;
			info->label_false = old_label_false;
			info->label_break = old_label_break;
			info->label_continue = old_label_continue;
		}
		break;
		// TODO: проверялось, только если в for присутствуют все блоки: инициализация, условие, модификация
		// нужно проверить и реализовать случаи, когда какие-нибудь из этих блоков отсутсвуют
		case OP_FOR:
		{
			const item_t ref_from = node_get_arg(nd, 0);
			const item_t ref_cond = node_get_arg(nd, 1);
			const item_t ref_incr = node_get_arg(nd, 2);
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;
			const item_t old_label_break = info->label_break;
			const item_t old_label_continue = info->label_continue;
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_incr = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_body;
			info->label_false = label_end;
			info->label_break = label_end;
			info->label_continue = label_body;

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

			info->label_true = old_label_true;
			info->label_false = old_label_false;
			info->label_break = old_label_break;
			info->label_continue = old_label_continue;
		}
		break;
		case OP_LABEL:
		{
			const item_t label = -node_get_arg(nd, 0);
			node_set_next(nd);
			to_code_unconditional_branch(info, label);
			to_code_label(info, label);
			statement(info, nd);
		}
		break;
		case OP_BREAK:
		{
			node_set_next(nd);
			to_code_unconditional_branch(info, info->label_break);
		}
		break;
		case OP_CONTINUE:
		{
			node_set_next(nd);
			to_code_unconditional_branch(info, info->label_continue);
		}
		break;
		case OP_GOTO:
		{
			const item_t label = node_get_arg(nd, 0) < 0 ? node_get_arg(nd, 0) : -node_get_arg(nd, 0);
			node_set_next(nd);
			to_code_unconditional_branch(info, label);
		}
		break;
		case OP_RETURN_VOID:
		{
			if (info->was_return)
			{
				info->was_return = 0;
				node_set_next(nd);
				break;
			}

			if (info->was_dynamic)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			uni_printf(info->sx->io, " ret void\n");
		}
		break;
		case OP_RETURN_VAL:
		{
			info->was_return = 1;

			if (info->was_dynamic)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			info->variable_location = LREG;
			expression(info, nd);

			// TODO: добавить обработку других ответов (ALOGIC)
			if (info->answer_type == ACONST && type_is_integer(info->answer_value_type))
			{
				uni_printf(info->sx->io, " ret i32 %" PRIitem "\n", info->answer_const);
			}
			else if (info->answer_type == ACONST && type_is_floating(info->answer_value_type))
			{
				uni_printf(info->sx->io, " ret double %f\n", info->answer_const_double);
			}
			else if (info->answer_type == AREG)
			{
				uni_printf(info->sx->io, " ret ");
				type_to_io(info, info->answer_value_type);
				uni_printf(info->sx->io, " %%.%" PRIitem "\n", info->answer_reg);
			}
			node_set_next(nd); // OP_RETURN_VOID
		}
		break;
		case OP_GETID:
			// здесь будет печать llvm для ввода
			node_set_next(nd);
			break;
		case OP_PRINTID:
			// здесь будет печать llvm для вывода
			node_set_next(nd);
			break;
		case OP_PRINTF:
		{
			const item_t N = node_get_amount(nd) - 1;
			item_t args[128];
			item_t args_type[128];

			node_set_next(nd);
			const item_t string_length = node_get_argc(nd) - 4;
			node_set_next(nd); // OP_STRING
			for (item_t i = 0; i < N; i++)
			{
				info->variable_location = LREG;
				expression(info, nd);
				args[i] = info->answer_reg;
				args_type[i] = info->answer_value_type;
			}

			uni_printf(info->sx->io, " %%.%" PRIitem " = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
				"([%" PRIitem " x i8], [%" PRIitem " x i8]* @.str%" PRIitem ", i32 0, i32 0)"
				, info->register_num
				, string_length + 1
				, string_length + 1
				, info->string_num);

			info->register_num++;
			info->string_num++;

			for (item_t i = 0; i < N; i++)
			{
				uni_printf(info->sx->io, ", ");
				type_to_io(info, args_type[i]);
				uni_printf(info->sx->io, " signext %%.%" PRIitem, args[i]);
			}

			uni_printf(info->sx->io, ")\n");
		}
		break;
		default:
			expression(info, nd);
			break;
	}
}

static void init(information *const info, node *const nd, const item_t displ, const item_t elem_type)
{
	// TODO: пока реализовано только для одномерных массивов
	if (node_get_type(nd) == OP_LIST && type_is_array(info->sx, expression_get_type(nd)))
	{
		const item_t N = node_get_arg(nd, 0);

		const size_t index = hash_get_index(&info->arrays, displ);
		hash_set_by_index(&info->arrays, index, 1, N);
		to_code_alloc_array_static(info, index, elem_type == TYPE_INTEGER ? TYPE_INTEGER : TYPE_FLOATING);
		to_code_init_array(info, index, elem_type == TYPE_INTEGER ? TYPE_INTEGER : TYPE_FLOATING);

		node_set_next(nd);
		for (item_t i = 0; i < N; i++)
		{
			info->variable_location = LFREE;
			expression(info, nd);
		}
	}
	else if (node_get_type(nd) == OP_STRUCT_INIT)
	{
		const item_t N = node_get_arg(nd, 0);

		node_set_next(nd);
		for (item_t i = 0; i < N; i++)
		{
			expression(info, nd);
		}
	}
	else
	{
		expression(info, nd);
	}
}

static void block(information *const info, node *const nd)
{
	const size_t block_size = node_get_amount(nd);
	node_set_next(nd); // OP_BLOCK

	for (size_t i = 0; i < block_size; i++)
	{
		switch (node_get_type(nd))
		{
			// case OP_DECL_ARR:
			// {
			// 	node id = node_get_child(nd, node_get_amount(nd) - 1);
			// 	if (node_get_type(&id) != OP_DECL_ID)
			// 	{
			// 		id = node_get_child(nd, node_get_amount(nd) - 2);
			// 	}

			// 	const item_t displ = node_get_arg(&id, 0);
			// 	const item_t elem_type = node_get_arg(&id, 1);
			// 	const size_t N = (size_t)node_get_arg(&id, 2);
			// 	const bool has_initializer = node_get_arg(&id, 3) != 0; // 0 если нет инициализации
			// 	const size_t index = hash_add(&info->arrays, displ, 1 + N);
			// 	hash_set_by_index(&info->arrays, index, IS_STATIC, 1);

			// 	node_set_next(nd);
			// 	// получение и сохранение границ
				// const bool has_bounds = node_get_type(nd) != OP_DECL_ID;
				// for (size_t i = 1; i <= N && has_bounds; i++)
				// {
				// 	info->variable_location = LFREE;
				// 	expression(info, nd);

				// 	if (!has_initializer)
				// 	{
				// 		if (info->answer_type == ACONST)
				// 		{
				// 			if (!hash_get_by_index(&info->arrays, index, IS_STATIC))
				// 			{
				// 				system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
				// 			}

				// 			hash_set_by_index(&info->arrays, index, i, info->answer_const);
				// 		}
				// 		else // if (info->answer_type == AREG) динамический массив
				// 		{
				// 			if (hash_get_by_index(&info->arrays, index, IS_STATIC) && i > 1)
				// 			{
				// 				system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
				// 			}

				// 			hash_set_by_index(&info->arrays, index, i, info->answer_reg);
				// 			hash_set_by_index(&info->arrays, index, IS_STATIC, 0);
				// 		}
				// 	}
				// }
	// 			node_set_next(nd);	// OP_DECL_ID

	// 			// объявление массива без инициализации, с инициализацией объявление происходит в init
	// 			// объявление массива, если он статический
	// 			if (hash_get_by_index(&info->arrays, index, IS_STATIC) && !has_initializer)
	// 			{
	// 				to_code_alloc_array_static(info, index, elem_type == mode_integer ? mode_integer : mode_float);
	// 			}
	// 			else if (!has_initializer) // объявление массива, если он динамический
	// 			{
	// 				if (!info->was_dynamic)
	// 				{
	// 					to_code_stack_save(info);
	// 				}
	// 				to_code_alloc_array_dynamic(info, index, elem_type == mode_integer ? mode_integer : mode_float);
	// 				info->was_dynamic = 1;
	// 			}

	// 			if (has_initializer)
	// 			{
	// 				init(info, nd, displ, elem_type);
	// 			}
			// }
			// break;
			case OP_DECL_ID:
			{
				const item_t displ = node_get_arg(nd, 0);
				const item_t elem_type = node_get_arg(nd, 1);
				const item_t N = node_get_arg(nd, 2);
				const item_t all = node_get_arg(nd, 3);

				if (N == 0) // обычная переменная int a; или struct point p;
				{
					uni_printf(info->sx->io, " %%var.%" PRIitem " = alloca ", displ);
					type_to_io(info, elem_type);
					uni_printf(info->sx->io, ", align 4\n");

					info->variable_location = LMEM;
					info->request_reg = displ;
				}

				node_set_next(nd);
				if (all)
				{
					init(info, nd, displ, elem_type);
				}
			}
			break;
			case OP_NOP:
			case OP_DECL_STRUCT:
			case OP_DECL_STRUCT_END:
				node_set_next(nd);
				break;
			default:
				statement(info, nd);
				break;
		}
	}
}

static int codegen(information *const info)
{
	int was_stack_functions = 0;
	node root = node_get_root(&info->sx->tree);

	while (node_set_next(&root) == 0)
	{
		switch (node_get_type(&root))
		{
			case OP_FUNC_DEF:
			{
				const size_t ref_ident = (size_t)node_get_arg(&root, 0);
				const item_t func_type = type_get(info->sx, (size_t)ident_get_type(info->sx, ref_ident) + 1);
				const size_t parameters = (size_t)type_get(info->sx, (size_t)ident_get_type(info->sx, ref_ident) + 2);
				info->was_dynamic = 0;

				if (ident_get_prev(info->sx, ref_ident) == TK_MAIN)
				{
					uni_printf(info->sx->io, "define i32 @main(");
				}
				else
				{
					uni_printf(info->sx->io, "define ");
					type_to_io(info, func_type);
					uni_printf(info->sx->io, " @func%zi(", ref_ident);
				}
				for (size_t i = 0; i < parameters; i++)
				{
					uni_printf(info->sx->io, i == 0 ? "" : ", ");
					type_to_io(info, ident_get_type(info->sx, ref_ident + 4 * (i + 1)));
				}
				uni_printf(info->sx->io, ") {\n");

				for (size_t i = 0; i < parameters; i++)
				{
					uni_printf(info->sx->io, " %%var.%" PRIitem " = alloca "
						, ident_get_displ(info->sx, ref_ident + 4 * (i + 1)));
					type_to_io(info, ident_get_type(info->sx, ref_ident + 4 * (i + 1)));
					uni_printf(info->sx->io, ", align 4\n");

					uni_printf(info->sx->io, " store ");
					type_to_io(info, ident_get_type(info->sx, ref_ident + 4 * (i + 1)));
					uni_printf(info->sx->io, " %%%" PRIitem ", ", i);
					type_to_io(info, ident_get_type(info->sx, ref_ident + 4 * (i + 1)));
					uni_printf(info->sx->io, "* %%var.%" PRIitem ", align 4\n"
						, ident_get_displ(info->sx, ref_ident + 4 * (i + 1)));
				}

				node_set_next(&root);
				block(info, &root);
				uni_printf(info->sx->io, "}\n\n");

				was_stack_functions |= info->was_dynamic;
			}
			break;
			// default:
			// 	system_error(node_unexpected, node_get_type(&root));
			// 	return -1;
		}
	}

	if (was_stack_functions)
	{
		uni_printf(info->sx->io, "declare i8* @llvm.stacksave()\n");
		uni_printf(info->sx->io, "declare void @llvm.stackrestore(i8*)\n");
	}
	if (info->was_memcpy)
	{
		uni_printf(info->sx->io, "declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture writeonly, i8* nocapture readonly, i32, i32, i1)\n");
	}

	return 0;
}

static void structs_declaration(information *const info)
{
	const size_t types_size = vector_size(&info->sx->types);
	for (size_t i = 0; i < types_size; i++)
	{
		if (type_is_structure(info->sx, i) && i != 2)
		{
			uni_printf(info->sx->io, "%%struct_opt.%zi = type { ", i);

			const size_t fields = (size_t)type_get(info->sx, i + 2);
			for (size_t j = 0; j < fields; j += 2)
			{
				uni_printf(info->sx->io, j == 0 ? "" : ", ");
				type_to_io(info, type_get(info->sx, i + 3 + j));
			}

			uni_printf(info->sx->io, " }\n");
		}
	}
	uni_printf(info->sx->io, " \n");
}


/*
 *	 __	 __   __	 ______   ______	 ______	 ______   ______	 ______	 ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	\ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	 \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_llvm(const workspace *const ws, syntax *const sx)
{
	tables_and_tree("tree.txt", &(sx->identifiers), &(sx->types), &(sx->tree));
	if (optimize_for_llvm(ws, sx))
	{
		return -1;
	}

	information info;
	info.sx = sx;
	info.string_num = 1;
	info.register_num = 1;
	info.label_num = 1;
	info.init_num = 1;
	info.variable_location = LREG;
	info.request_reg = 0;
	info.answer_reg = 0;
	info.was_memcpy = 0;
	info.was_return = 0;

	info.arrays = hash_create(HASH_TABLE_SIZE);

	structs_declaration(&info);

	const int ret = codegen(&info);

	hash_clear(&info.arrays);
	return ret;
}
