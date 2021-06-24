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
#include "parser.h"
#include "tree.h"
#include "uniprinter.h"


const size_t HASH_TABLE_SIZE = 1024;
const size_t IS_STATIC = 0;


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

typedef struct information
{
	universal_io *io;					/**< Вывод */
	syntax *sx;							/**< Структура syntax с таблицами */

	item_t string_num;					/**< Номер строки */
	item_t register_num;				/**< Номер регистра */
	item_t label_num;					/**< Номер метки */

	item_t request_reg;					/**< Регистр на запрос */
	location_t variable_location;		/**< Расположение переменной */

	item_t answer_reg;					/**< Регистр с ответом */
	item_t answer_const;				/**< Константа с ответом */
	double answer_const_double;			/**< Константа с ответом типа double */
	answer_t answer_type;				/**< Тип ответа */
	item_t answer_value_type;			/**< Тип значения */

	item_t label_true;					/**< Метка перехода при true */
	item_t label_false;					/**< Метка перехода при false */

	hash arrays;						/**< Хеш таблица с информацией о массивах:
												@с key		 - смещение массива
												@c value[0]	 - флаг статичности
												@c value[1..MAX] - границы массива */
	int was_dynamic;					/**< Истина, если в функции были динамические массивы */
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

// TODO: помню, такое планировалось делать вне кодогенератора...
static int is_double(const item_t operation)
{
	switch (operation)
	{
		case OP_ASSIGN_R:
		case OP_ADD_ASSIGN_R:
		case OP_SUB_ASSIGN_R:
		case OP_MUL_ASSIGN_R:
		case OP_DIV_ASSIGN_R:

		case OP_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R:

		case OP_ASSIGN_R_V:
		case OP_ADD_ASSIGN_R_V:
		case OP_SUB_ASSIGN_R_V:
		case OP_MUL_ASSIGN_R_V:
		case OP_DIV_ASSIGN_R_V:

		case OP_ASSIGN_AT_R_V:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_MUL_ASSIGN_AT_R_V:
		case OP_DIV_ASSIGN_AT_R_V:

		case OP_EQ_R:
		case OP_NE_R:
		case OP_LT_R:
		case OP_GT_R:
		case OP_LE_R:
		case OP_GE_R:
		case OP_ADD_R:
		case OP_SUB_R:
		case OP_MUL_R:
		case OP_DIV_R:

		case OP_POST_INC_R:
		case OP_POST_DEC_R:
		case OP_PRE_INC_R:
		case OP_PRE_DEC_R:
		case OP_POST_INC_AT_R:
		case OP_POST_DEC_AT_R:
		case OP_PRE_INC_AT_R:
		case OP_PRE_DEC_AT_R:
		case OP_POST_INC_R_V:
		case OP_POST_DEC_R_V:
		case OP_PRE_INC_R_V:
		case OP_PRE_DEC_R_V:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R_V:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R_V:

		case OP_UNMINUS_R:
			return 1;

		default:
			return 0;
	}
}

static int is_array_operation(const item_t operation)
{
	switch (operation)
	{
		case OP_POST_INC_AT:
		case OP_POST_DEC_AT:
		case OP_PRE_INC_AT:
		case OP_PRE_DEC_AT:
		case OP_POST_INC_AT_V:
		case OP_POST_DEC_AT_V:
		case OP_PRE_INC_AT_V:
		case OP_PRE_DEC_AT_V:

		case OP_POST_INC_AT_R:
		case OP_POST_DEC_AT_R:
		case OP_PRE_INC_AT_R:
		case OP_PRE_DEC_AT_R:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R_V:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R_V:

		case OP_REM_ASSIGN_AT:
		case OP_SHL_ASSIGN_AT:
		case OP_SHR_ASSIGN_AT:
		case OP_AND_ASSIGN_AT:
		case OP_XOR_ASSIGN_AT:
		case OP_OR_ASSIGN_AT:

		case OP_ASSIGN_AT:
		case OP_ADD_ASSIGN_AT:
		case OP_SUB_ASSIGN_AT:
		case OP_MUL_ASSIGN_AT:
		case OP_DIV_ASSIGN_AT:

		case OP_REM_ASSIGN_AT_V:
		case OP_SHL_ASSIGN_AT_V:
		case OP_SHR_ASSIGN_AT_V:
		case OP_AND_ASSIGN_AT_V:
		case OP_XOR_ASSIGN_AT_V:
		case OP_OR_ASSIGN_AT_V:

		case OP_ASSIGN_AT_V:
		case OP_ADD_ASSIGN_AT_V:
		case OP_SUB_ASSIGN_AT_V:
		case OP_MUL_ASSIGN_AT_V:
		case OP_DIV_ASSIGN_AT_V:

		case OP_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R:

		case OP_ASSIGN_AT_R_V:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_MUL_ASSIGN_AT_R_V:
		case OP_DIV_ASSIGN_AT_R_V:
		return 1;

		default:
			return 0;
	}
}

static void type_to_io(universal_io *const io, const item_t type)
{
	if (mode_is_int(type))
	{
		uni_printf(io, "i32");
	}
	else if (mode_is_float(type))
	{
		uni_printf(io, "double");
	}
	else if (mode_is_void(type))
	{
		uni_printf(io, "void");
	}
}

static void operation_to_io(universal_io *const io, const item_t type)
{
	switch (type)
	{
		case OP_PRE_INC:
		case OP_PRE_INC_V:
		case OP_POST_INC:
		case OP_POST_INC_V:
		case OP_ADD_ASSIGN:
		case OP_ADD_ASSIGN_V:
		case OP_ADD:
		case OP_ADD_ASSIGN_AT:
		case OP_ADD_ASSIGN_AT_V:
		case OP_PRE_INC_AT:
		case OP_PRE_INC_AT_V:
		case OP_POST_INC_AT:
		case OP_POST_INC_AT_V:
			uni_printf(io, "add nsw");
			break;

		case OP_PRE_DEC:
		case OP_PRE_DEC_V:
		case OP_POST_DEC:
		case OP_POST_DEC_V:
		case OP_SUB_ASSIGN:
		case OP_SUB_ASSIGN_V:
		case OP_SUB:
		case OP_UNMINUS:
		case OP_SUB_ASSIGN_AT:
		case OP_SUB_ASSIGN_AT_V:
		case OP_PRE_DEC_AT:
		case OP_PRE_DEC_AT_V:
		case OP_POST_DEC_AT:
		case OP_POST_DEC_AT_V:
			uni_printf(io, "sub nsw");
			break;

		case OP_MUL_ASSIGN:
		case OP_MUL_ASSIGN_V:
		case OP_MUL:
		case OP_MUL_ASSIGN_AT:
		case OP_MUL_ASSIGN_AT_V:
			uni_printf(io, "mul nsw");
			break;

		case OP_DIV_ASSIGN:
		case OP_DIV_ASSIGN_V:
		case OP_DIV:
		case OP_DIV_ASSIGN_AT:
		case OP_DIV_ASSIGN_AT_V:
			uni_printf(io, "sdiv");
			break;

		case OP_REM_ASSIGN:
		case OP_REM_ASSIGN_V:
		case OP_REM:
		case OP_REM_ASSIGN_AT:
		case OP_REM_ASSIGN_AT_V:
			uni_printf(io, "srem");
			break;

		case OP_SHL_ASSIGN:
		case OP_SHL_ASSIGN_V:
		case OP_SHL:
		case OP_SHL_ASSIGN_AT:
		case OP_SHL_ASSIGN_AT_V:
			uni_printf(io, "shl");
			break;

		case OP_SHR_ASSIGN:
		case OP_SHR_ASSIGN_V:
		case OP_SHR:
		case OP_SHR_ASSIGN_AT:
		case OP_SHR_ASSIGN_AT_V:
			uni_printf(io, "ashr");
			break;

		case OP_AND_ASSIGN:
		case OP_AND_ASSIGN_V:
		case OP_AND:
		case OP_AND_ASSIGN_AT:
		case OP_AND_ASSIGN_AT_V:
			uni_printf(io, "and");
			break;

		case OP_XOR_ASSIGN:
		case OP_XOR_ASSIGN_V:
		case OP_XOR:
		case OP_NOT:
		case OP_XOR_ASSIGN_AT:
		case OP_XOR_ASSIGN_AT_V:
			uni_printf(io, "xor");
			break;

		case OP_OR_ASSIGN:
		case OP_OR_ASSIGN_V:
		case OP_OR:
		case OP_OR_ASSIGN_AT:
		case OP_OR_ASSIGN_AT_V:
			uni_printf(io, "or");
			break;

		case OP_EQ:
			uni_printf(io, "icmp eq");
			break;
		case OP_NE:
			uni_printf(io, "icmp ne");
			break;
		case OP_LT:
			uni_printf(io, "icmp slt");
			break;
		case OP_GT:
			uni_printf(io, "icmp sgt");
			break;
		case OP_LE:
			uni_printf(io, "icmp sle");
			break;
		case OP_GE:
			uni_printf(io, "icmp sge");
			break;

		case OP_PRE_INC_R:
		case OP_PRE_INC_R_V:
		case OP_POST_INC_R:
		case OP_POST_INC_R_V:
		case OP_ADD_ASSIGN_R:
		case OP_ADD_ASSIGN_R_V:
		case OP_ADD_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_PRE_INC_AT_R:
		case OP_PRE_INC_AT_R_V:
		case OP_POST_INC_AT_R:
		case OP_POST_INC_AT_R_V:
			uni_printf(io, "fadd");
			break;

		case OP_PRE_DEC_R:
		case OP_PRE_DEC_R_V:
		case OP_POST_DEC_R:
		case OP_POST_DEC_R_V:
		case OP_SUB_ASSIGN_R:
		case OP_SUB_ASSIGN_R_V:
		case OP_SUB_R:
		case OP_UNMINUS_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_PRE_DEC_AT_R:
		case OP_PRE_DEC_AT_R_V:
		case OP_POST_DEC_AT_R:
		case OP_POST_DEC_AT_R_V:
			uni_printf(io, "fsub");
			break;

		case OP_MUL_ASSIGN_R:
		case OP_MUL_ASSIGN_R_V:
		case OP_MUL_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R_V:
			uni_printf(io, "fmul");
			break;

		case OP_DIV_ASSIGN_R:
		case OP_DIV_ASSIGN_R_V:
		case OP_DIV_R:
		case OP_DIV_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R_V:
			uni_printf(io, "fdiv");
			break;

		case OP_EQ_R:
			uni_printf(io, "fcmp oeq");
			break;
		case OP_NE_R:
			uni_printf(io, "fcmp one");
			break;
		case OP_LT_R:
			uni_printf(io, "fcmp olt");
			break;
		case OP_GT_R:
			uni_printf(io, "fcmp ogt");
			break;
		case OP_LE_R:
			uni_printf(io, "fcmp ole");
			break;
		case OP_GE_R:
			uni_printf(io, "fcmp oge");
			break;
	}
}

static void to_code_operation_reg_reg(information *const info, const item_t operation
	, const item_t fst, const item_t snd, const item_t type)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, operation);
	uni_printf(info->io, " ");
	type_to_io(info->io, type);
	uni_printf(info->io, " %%.%" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_operation_reg_const_i32(information *const info, const item_t operation
	, const item_t fst, const item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, operation);
	uni_printf(info->io, " i32 %%.%" PRIitem ", %" PRIitem "\n", fst, snd);
}

static void to_code_operation_reg_const_double(information *const info, const item_t operation
	, const item_t fst, double snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, operation);
	uni_printf(info->io, " double %%.%" PRIitem ", %f\n", fst, snd);
}

static void to_code_operation_const_reg_i32(information *const info, const item_t operation
	, const item_t fst, const item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, operation);
	uni_printf(info->io, " i32 %" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_operation_const_reg_double(information *const info, const item_t operation
	, const double fst, const item_t snd)
{
	uni_printf(info->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->io, operation);
	uni_printf(info->io, " double %f, %%.%" PRIitem "\n", fst, snd);
}

static void to_code_load(information *const info, const item_t result, const item_t displ, const item_t type
	, const int is_array)
{
	uni_printf(info->io, " %%.%" PRIitem " = load ", result);
	type_to_io(info->io, type);
	uni_printf(info->io, ", ");
	type_to_io(info->io, type);
	uni_printf(info->io, "* %%%s.%" PRIitem ", align 4\n", is_array ? "" : "var", displ);
}

static inline void to_code_store_reg(information *const info, const item_t reg, const item_t displ, const item_t type
	, const int is_array)
{
	uni_printf(info->io, " store ");
	type_to_io(info->io, type);
	uni_printf(info->io, " %%.%" PRIitem ", ", reg);
	type_to_io(info->io, type);
	uni_printf(info->io, "* %%%s.%" PRIitem ", align 4\n", is_array ? "" : "var", displ);
}

static inline void to_code_store_const_i32(information *const info, const item_t arg, const item_t displ
	, const int is_array)
{
	uni_printf(info->io, " store i32 %" PRIitem ", i32* %%%s.%" PRIitem ", align 4\n"
		, arg, is_array ? "" : "var", displ);
}

static inline void to_code_store_const_double(information *const info, const double arg, const item_t displ
	, const int is_array)
{
	uni_printf(info->io, " store double %f, double* %%%s.%" PRIitem ", align 4\n", arg, is_array ? "" : "var", displ);
}

static void to_code_try_zext_to(information *const info)
{
	if (info->answer_type != ALOGIC)
	{
		return;
	}

	uni_printf(info->io, " %%.%" PRIitem " = zext i1 %%.%" PRIitem " to i32\n", info->register_num, info->answer_reg);
	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
}

static inline void to_code_label(information *const info, const item_t label_num)
{
	uni_printf(info->io, " label%" PRIitem ":\n", label_num);
}

static inline void to_code_unconditional_branch(information *const info, const item_t label_num)
{
	uni_printf(info->io, " br label %%label%" PRIitem "\n", label_num);
}

static inline void to_code_conditional_branch(information *const info)
{
	uni_printf(info->io, " br i1 %%.%" PRIitem ", label %%label%" PRIitem ", label %%label%" PRIitem "\n"
		, info->answer_reg, info->label_true, info->label_false);
}

static void to_code_alloc_array_static(information *const info, const size_t index, const item_t type)
{
	uni_printf(info->io, " %%arr.%" PRIitem " = alloca ", hash_get_key(&info->arrays, index));

	const size_t dim = hash_get_amount_by_index(&info->arrays, index) - 1;
	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->io, "[%" PRIitem " x ", hash_get_by_index(&info->arrays, index, i));
	}
	type_to_io(info->io, type);

	for (size_t i = 1; i <= dim; i++)
	{
		uni_printf(info->io, "]");
	}
	uni_printf(info->io, ", align 4\n");
}

static void to_code_alloc_array_dynamic(information *const info, const size_t index, const item_t type)
{
	// выделение памяти на стеке
	item_t to_alloc = hash_get_by_index(&info->arrays, index, 1);

	const size_t dim = hash_get_amount_by_index(&info->arrays, index) - 1;
	for (size_t i = 2; i <= dim; i++)
	{
		uni_printf(info->io, " %%.%" PRIitem " = mul nuw i32 %%.%" PRIitem ", %%.%" PRIitem "\n"
			, info->register_num, to_alloc, hash_get_by_index(&info->arrays, index, i));
		to_alloc = info->register_num++;
	}
	uni_printf(info->io, " %%dynarr.%" PRIitem " = alloca ", hash_get_key(&info->arrays, index));
	type_to_io(info->io, type);
	uni_printf(info->io, ", i32 %%.%" PRIitem ", align 4\n", to_alloc);
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

static void to_code_slice(information *const info, const item_t displ, const item_t cur_dimension
	, const item_t prev_slice, const item_t type)
{
	uni_printf(info->io, " %%.%" PRIitem " = getelementptr inbounds ", info->register_num);
	const item_t dimensions = hash_get_amount(&info->arrays, displ) - 1;

	if (hash_get(&info->arrays, displ, IS_STATIC))
	{
		for (item_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->io, "[%" PRIitem " x ", hash_get(&info->arrays, displ, (size_t)i));
		}
		type_to_io(info->io, type);

		for (item_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->io, "]");
		}
		uni_printf(info->io, ", ");

		for (item_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->io, "[%" PRIitem " x ", hash_get(&info->arrays, displ, (size_t)i));
		}
		type_to_io(info->io, type);

		for (item_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->io, "]");
		}

		if (cur_dimension == dimensions - 1)
		{
			uni_printf(info->io, "* %%arr.%" PRIitem ", i32 0", displ);
		}
		else
		{
			uni_printf(info->io, "* %%.%" PRIitem ", i32 0", prev_slice);
		}
	}
	else if (cur_dimension == dimensions - 1)
	{
		type_to_io(info->io, type);
		uni_printf(info->io, ", ");
		type_to_io(info->io, type);
		uni_printf(info->io, "* %%dynarr.%" PRIitem, displ);
	}
	else
	{
		type_to_io(info->io, type);
		uni_printf(info->io, ", ");
		type_to_io(info->io, type);
		uni_printf(info->io, "* %%.%" PRIitem, prev_slice);
	}

	if (info->answer_type == AREG)
	{
		uni_printf(info->io, ", i32 %%.%" PRIitem "\n", info->answer_reg);
	}
	else // if (info->answer_type == ACONST)
	{
		uni_printf(info->io, ", i32 %" PRIitem "\n", info->answer_const);
	}

	info->register_num++;
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
			to_code_operation_reg_const_i32(info, OP_NE, info->answer_reg, 0);
			info->answer_reg = info->register_num++;
		}
		case ALOGIC:
			to_code_conditional_branch(info);
			break;
	}
}


static void operand(information *const info, node *const nd)
{
	if (node_get_type(nd) == OP_NOP || node_get_type(nd) == OP_AD_LOG_OR || node_get_type(nd) == OP_AD_LOG_AND)
	{
		node_set_next(nd);
	}

	switch (node_get_type(nd))
	{
		case OP_IDENT:
		case OP_SELECT:
		case OP_IDENT_TO_ADDR:
			node_set_next(nd);
			break;
		case OP_IDENT_TO_VAL:
		{
			const item_t displ = node_get_arg(nd, 0);

			to_code_load(info, info->register_num, displ, mode_integer, 0);
			info->answer_reg = info->register_num++;
			info->answer_type = AREG;
			info->answer_value_type = mode_integer;
			node_set_next(nd);
		}
		break;
		case OP_IDENT_TO_VAL_D:
		{
			const item_t displ = node_get_arg(nd, 0);

			to_code_load(info, info->register_num, displ, mode_float, 0);
			info->answer_reg = info->register_num++;
			info->answer_type = AREG;
			info->answer_value_type = mode_float;
			node_set_next(nd);
		}
		break;
		case OP_CONST:
		{
			const item_t num = node_get_arg(nd, 0);

			if (info->variable_location == LMEM)
			{
				to_code_store_const_i32(info, num, info->request_reg, 0);
				info->answer_type = AREG;
			}
			else
			{
				info->answer_type = ACONST;
				info->answer_const = num;
				info->answer_value_type = mode_integer;
			}

			node_set_next(nd);
		}
		break;
		case OP_CONST_D:
		{
			const double num = to_double(node_get_arg(nd, 0), node_get_arg(nd, 1));

			if (info->variable_location == LMEM)
			{
				to_code_store_const_double(info, num, info->request_reg, 0);
				info->answer_type = AREG;
			}
			else
			{
				info->answer_type = ACONST;
				info->answer_const_double = num;
				info->answer_value_type = mode_float;
			}

			node_set_next(nd);
		}
		break;
		case OP_STRING:
			node_set_next(nd);
			break;
		case OP_SLICE_IDENT:
		{
			const item_t displ = node_get_arg(nd, 0);
			const item_t type = node_get_arg(nd, 1);
			item_t cur_dimension = hash_get_amount(&info->arrays, displ) - 2;
			const location_t location = info->variable_location;
			node_set_next(nd);

			info->variable_location = LFREE;
			expression(info, nd);

			// TODO: пока только для динамических массивов размерности 2
			if (!hash_get(&info->arrays, displ, IS_STATIC) && cur_dimension == 1)
			{
				if (info->answer_type == ACONST)
				{
					to_code_operation_const_reg_i32(info, OP_MUL, info->answer_const, hash_get(&info->arrays, displ, 2));
				}
				else // if (info->answer_type == AREG)
				{
					to_code_operation_reg_reg(info, OP_MUL, info->answer_reg, hash_get(&info->arrays, displ, 2),
						mode_integer);
				}

				info->answer_type = AREG;
				info->answer_reg = info->register_num++;
			}

			to_code_slice(info, displ, cur_dimension, 0, type);

			item_t prev_slice = info->register_num - 1;
			while (node_get_type(nd) == OP_SLICE)
			{
				node_set_next(nd);
				info->variable_location = LFREE;
				expression(info, nd);
				cur_dimension--;

				to_code_slice(info, displ, cur_dimension, prev_slice, type);
				prev_slice = info->register_num - 1;
			}

			// TODO: может это замена LMEM? Подумать, когда будут реализовываться указатели
			if (node_get_type(nd) == OP_ADDR_TO_VAL)
			{
				node_set_next(nd);
			}

			if (location != LMEM)
			{
				to_code_load(info, info->register_num, info->register_num - 1, type, 1);
				info->register_num++;
			}

			info->answer_reg = info->register_num - 1;
			info->answer_type = AREG;
			info->answer_value_type = type;
		}
		break;
		case OP_CALL1:
		{
			const item_t args = node_get_arg(nd, 0);

			node_set_next(nd);
			for (item_t i = 0; i < args; i++)
			{
				expression(info, nd);
			}

			const size_t ref_ident = (size_t)node_get_arg(nd, 0);
			const item_t func_type = mode_get(info->sx, (size_t)ident_get_mode(info->sx, ref_ident) + 1);

			node_set_next(nd); // OP_CALL2

			if (func_type != mode_void)
			{
				uni_printf(info->io, " %%.%" PRIitem " =", info->register_num);
				info->answer_type = AREG;
				info->answer_value_type = func_type;
				info->answer_reg = info->register_num++;
			}
			uni_printf(info->io, " call ");
			type_to_io(info->io, func_type);
			uni_printf(info->io, " @func%zi(", ref_ident);

			// тут будет ещё перечисление аргументов
			uni_printf(info->io, ")\n");
		}
		break;
		case OP_ARRAY_INIT:
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
		case OP_STRUCT_INIT:
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
	const item_t operation_type = is_double(assignment_type) ? mode_float : mode_integer;
	const int is_array = is_array_operation(assignment_type);

	node_set_next(nd);
	info->variable_location = LMEM;
	operand(info, nd); // Tident or OP_SLICE_IDENT
	const item_t memory_reg = info->answer_reg;

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_zext_to(info);
	item_t result = info->answer_reg;

	if (assignment_type != OP_ASSIGN && assignment_type != OP_ASSIGN_V && assignment_type != OP_ASSIGN_R && assignment_type != OP_ASSIGN_R_V
		&& assignment_type != OP_ASSIGN_AT && assignment_type != OP_ASSIGN_AT_V
    && assignment_type != OP_ASSIGN_AT_R && assignment_type != OP_ASSIGN_AT_R_V)
	{
		to_code_load(info, info->register_num, is_array ? memory_reg : displ, operation_type, is_array);
		info->register_num++;

		if (info->answer_type == AREG)
		{
			to_code_operation_reg_reg(info, assignment_type, info->register_num - 1, info->answer_reg, operation_type);
		}
		// ACONST
		else if (!is_double(assignment_type))
		{
			to_code_operation_reg_const_i32(info, assignment_type, info->register_num - 1, info->answer_const);
		}
		else
		{
			to_code_operation_reg_const_double(info, assignment_type, info->register_num - 1
				, info->answer_const_double);
		}

		result = info->register_num++;
		info->answer_type = AREG;
	}

	if (info->answer_type == AREG)
	{
		to_code_store_reg(info, result, is_array ? memory_reg : displ, operation_type, is_array);
	}
	// ACONST && =
	else if (!is_double(assignment_type))
	{
		to_code_store_const_i32(info, info->answer_const, is_array ? memory_reg : displ, is_array);
	}
	else
	{
		to_code_store_const_double(info, info->answer_const_double, is_array ? memory_reg : displ, is_array);
	}
}

static void integral_expression(information *const info, node *const nd, const answer_t type)
{
	const item_t operation = node_get_type(nd);
	const item_t operation_type = is_double(operation) ? mode_float : mode_integer;
	node_set_next(nd);

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_zext_to(info);

	const answer_t left_type = info->answer_type;
	const item_t left_reg = info->answer_reg;
	const item_t left_const = info->answer_const;
	const double left_const_double = info->answer_const_double;

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_zext_to(info);

	const answer_t right_type = info->answer_type;
	const item_t right_reg = info->answer_reg;
	const item_t right_const = info->answer_const;
	const double right_const_double = info->answer_const_double;

	info->answer_value_type = type != ALOGIC ? operation_type : mode_integer;

	if (left_type == AREG && right_type == AREG)
	{
		to_code_operation_reg_reg(info, operation, left_reg, right_reg, operation_type);
	}
	else if (left_type == AREG && right_type == ACONST && !is_double(operation))
	{
		to_code_operation_reg_const_i32(info, operation, left_reg, right_const);
	}
	else if (left_type == AREG && right_type == ACONST) // double
	{
		to_code_operation_reg_const_double(info, operation, left_reg, right_const_double);
	}
	else if (left_type == ACONST && right_type == AREG && !is_double(operation))
	{
		to_code_operation_const_reg_i32(info, operation, left_const, right_reg);
	}
		else if (left_type == ACONST && right_type == AREG) // double
	{
		to_code_operation_const_reg_double(info, operation, left_const_double, right_reg);
	}
	else // if (left_type == ACONST && right_type == ACONST)
	{
		info->answer_type = ACONST;

		switch (operation)
		{
			case OP_ADD:
				info->answer_const = left_const + right_const;
				break;
			case OP_SUB:
				info->answer_const = left_const - right_const;
				break;
			case OP_MUL:
				info->answer_const = left_const * right_const;
				break;
			case OP_DIV:
				info->answer_const = left_const / right_const;
				break;
			case OP_REM:
				info->answer_const = left_const % right_const;
				break;
			case OP_SHL:
				info->answer_const = left_const << right_const;
				break;
			case OP_SHR:
				info->answer_const = left_const >> right_const;
				break;
			case OP_AND:
				info->answer_const = left_const & right_const;
				break;
			case OP_XOR:
				info->answer_const = left_const ^ right_const;
				break;
			case OP_OR:
				info->answer_const = left_const | right_const;
				break;

			case OP_EQ:
				info->answer_const = left_const == right_const;
				break;
			case OP_NE:
				info->answer_const = left_const != right_const;
				break;
			case OP_LT:
				info->answer_const = left_const < right_const;
				break;
			case OP_GT:
				info->answer_const = left_const > right_const;
				break;
			case OP_LE:
				info->answer_const = left_const <= right_const;
				break;
			case OP_GE:
				info->answer_const = left_const >= right_const;
				break;

			case OP_ADD_R:
				info->answer_const_double = left_const_double + right_const_double;
				break;
			case OP_SUB_R:
				info->answer_const_double = left_const_double - right_const_double;
				break;
			case OP_MUL_R:
				info->answer_const_double = left_const_double * right_const_double;
				break;
			case OP_DIV_R:
				info->answer_const_double = left_const_double / right_const_double;
				break;

			case OP_EQ_R:
				info->answer_const = left_const_double == right_const_double;
				break;
			case OP_NE_R:
				info->answer_const = left_const_double != right_const_double;
				break;
			case OP_LT_R:
				info->answer_const = left_const_double < right_const_double;
				break;
			case OP_GT_R:
				info->answer_const = left_const_double > right_const_double;
				break;
			case OP_LE_R:
				info->answer_const = left_const_double <= right_const_double;
				break;
			case OP_GE_R:
				info->answer_const = left_const_double >= right_const_double;
				break;
		}
		return;
	}

	info->answer_reg = info->register_num++;
	info->answer_type = type;
}

// Обрабатываются операции инкремента/декремента и постинкремента/постдекремента
static void inc_dec_expression(information *const info, node *const nd)
{
	const item_t displ = node_get_arg(nd, 0);
	const item_t operation = node_get_type(nd);
	const item_t operation_type = is_double(operation) ? mode_float : mode_integer;

	node_set_next(nd);
	info->variable_location = LMEM;
	operand(info, nd); // Tident or OP_SLICE_IDENT
	const item_t memory_reg = info->answer_reg;

	to_code_load(info, info->register_num, is_array_operation(operation) ? memory_reg : displ, operation_type
		, is_array_operation(operation));
	info->answer_type = AREG;
	info->answer_reg = info->register_num++;
	info->answer_value_type = operation_type;

	switch (operation)
	{
		case OP_PRE_INC:
		case OP_PRE_INC_V:
		case OP_PRE_DEC:
		case OP_PRE_DEC_V:

		case OP_PRE_INC_AT:
		case OP_PRE_INC_AT_V:
		case OP_PRE_DEC_AT:
		case OP_PRE_DEC_AT_V:
			info->answer_reg = info->register_num;
		case OP_POST_INC:
		case OP_POST_INC_V:
		case OP_POST_DEC:
		case OP_POST_DEC_V:

		case OP_POST_INC_AT:
		case OP_POST_INC_AT_V:
		case OP_POST_DEC_AT:
		case OP_POST_DEC_AT_V:
			to_code_operation_reg_const_i32(info, operation, info->register_num - 1, 1);
			break;

		case OP_PRE_INC_R:
		case OP_PRE_INC_R_V:
		case OP_PRE_DEC_R:
		case OP_PRE_DEC_R_V:

		case OP_PRE_INC_AT_R:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R:
		case OP_PRE_DEC_AT_R_V:
			info->answer_reg = info->register_num;
		case OP_POST_INC_R:
		case OP_POST_INC_R_V:
		case OP_POST_DEC_R:
		case OP_POST_DEC_R_V:

		case OP_POST_INC_AT_R:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R:
		case OP_POST_DEC_AT_R_V:
			to_code_operation_reg_const_double(info, operation, info->register_num - 1, 1.0);
			break;
	}

	to_code_store_reg(info, info->register_num, is_array_operation(operation) ? memory_reg : displ, operation_type
		, is_array_operation(operation));
	info->register_num++;
}

static void unary_operation(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_POST_INC:
		case OP_POST_INC_V:
		case OP_POST_DEC:
		case OP_POST_DEC_V:
		case OP_PRE_INC:
		case OP_PRE_INC_V:
		case OP_PRE_DEC:
		case OP_PRE_DEC_V:

		case OP_POST_INC_AT:
		case OP_POST_INC_AT_V:
		case OP_POST_DEC_AT:
		case OP_POST_DEC_AT_V:
		case OP_PRE_INC_AT:
		case OP_PRE_INC_AT_V:
		case OP_PRE_DEC_AT:
		case OP_PRE_DEC_AT_V:

		case OP_POST_INC_R:
		case OP_POST_INC_R_V:
		case OP_POST_DEC_R:
		case OP_POST_DEC_R_V:
		case OP_PRE_INC_R:
		case OP_PRE_INC_R_V:
		case OP_PRE_DEC_R:
		case OP_PRE_DEC_R_V:

		case OP_POST_INC_AT_R:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R:
		case OP_POST_DEC_AT_R_V:
		case OP_PRE_INC_AT_R:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R:
		case OP_PRE_DEC_AT_R_V:
			inc_dec_expression(info, nd);
			break;
		case OP_UNMINUS:
		case OP_NOT:
		case OP_UNMINUS_R:
		{
			const item_t operation_type = node_get_type(nd);
			node_set_next(nd);

			info->variable_location = LREG;
			expression(info, nd);

			to_code_try_zext_to(info);

			info->answer_value_type = mode_integer;
			if (operation_type == OP_UNMINUS)
			{
				to_code_operation_const_reg_i32(info, OP_UNMINUS, 0, info->answer_reg);
			}
			else if (operation_type == OP_NOT)
			{
				to_code_operation_reg_const_i32(info, OP_NOT, info->answer_reg, -1);
			}
			else // OP_UNMINUS_R
			{
				to_code_operation_const_reg_double(info, OP_UNMINUS_R, 0, info->answer_reg);
				info->answer_value_type = mode_float;
			}

			info->answer_type = AREG;
			info->answer_reg = info->register_num++;
		}
		break;
		case OP_LOG_NOT:
		{
			const item_t temp = info->label_true;
			info->label_true =  info->label_false;
			info->label_false = temp;

			node_set_next(nd);
			expression(info, nd);
		}
		break;
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
		case OP_ASSIGN:
		case OP_ASSIGN_V:

		case OP_ADD_ASSIGN:
		case OP_ADD_ASSIGN_V:
		case OP_SUB_ASSIGN:
		case OP_SUB_ASSIGN_V:
		case OP_MUL_ASSIGN:
		case OP_MUL_ASSIGN_V:
		case OP_DIV_ASSIGN:
		case OP_DIV_ASSIGN_V:

		case OP_REM_ASSIGN:
		case OP_REM_ASSIGN_V:
		case OP_SHL_ASSIGN:
		case OP_SHL_ASSIGN_V:
		case OP_SHR_ASSIGN:
		case OP_SHR_ASSIGN_V:
		case OP_AND_ASSIGN:
		case OP_AND_ASSIGN_V:
		case OP_XOR_ASSIGN:
		case OP_XOR_ASSIGN_V:
		case OP_OR_ASSIGN:
		case OP_OR_ASSIGN_V:

		case OP_ASSIGN_R:
		case OP_ASSIGN_R_V:

		case OP_ADD_ASSIGN_R:
		case OP_ADD_ASSIGN_R_V:
		case OP_SUB_ASSIGN_R:
		case OP_SUB_ASSIGN_R_V:
		case OP_MUL_ASSIGN_R:
		case OP_MUL_ASSIGN_R_V:
		case OP_DIV_ASSIGN_R:
		case OP_DIV_ASSIGN_R_V:

		case OP_ASSIGN_AT:
		case OP_ADD_ASSIGN_AT:
		case OP_SUB_ASSIGN_AT:
		case OP_MUL_ASSIGN_AT:
		case OP_DIV_ASSIGN_AT:

		case OP_ASSIGN_AT_V:
		case OP_ADD_ASSIGN_AT_V:
		case OP_SUB_ASSIGN_AT_V:
		case OP_MUL_ASSIGN_AT_V:
		case OP_DIV_ASSIGN_AT_V:

		case OP_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R:

		case OP_ASSIGN_AT_R_V:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_MUL_ASSIGN_AT_R_V:
		case OP_DIV_ASSIGN_AT_R_V:
			assignment_expression(info, nd);
			break;


		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:

		case OP_REM:
		case OP_SHL:
		case OP_SHR:
		case OP_AND:
		case OP_XOR:
		case OP_OR:

		case OP_ADD_R:
		case OP_SUB_R:
		case OP_MUL_R:
		case OP_DIV_R:
			integral_expression(info, nd, AREG);
			break;


		case OP_EQ:
		case OP_NE:
		case OP_LT:
		case OP_GT:
		case OP_LE:
		case OP_GE:

		case OP_EQ_R:
		case OP_NE_R:
		case OP_LT_R:
		case OP_GT_R:
		case OP_LE_R:
		case OP_GE_R:
			integral_expression(info, nd, ALOGIC);
			break;

		// TODO: протестировать и при необходимости реализовать случай, когда && и || есть в арифметических выражениях
		case OP_LOG_OR:
		case OP_LOG_AND:
		{
			const item_t label_next = info->label_num++;
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;

			if (node_get_type(nd) == OP_LOG_OR)
			{
				info->label_false = label_next;
			}
			else // (node_get_type(nd) == OP_LOG_AND)
			{
				info->label_true = label_next;
			}

			node_set_next(nd);
			expression(info, nd);

			// TODO: сделать обработку других ответов
			// постараться использовать функцию check_type_and_branch
			if (info->answer_type == ALOGIC)
			{
				to_code_conditional_branch(info);
			}

			to_code_label(info, label_next);
			info->label_true = old_label_true;
			info->label_false = old_label_false;

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
		case OP_POST_INC:
		case OP_POST_DEC:
		case OP_PRE_INC:
		case OP_PRE_DEC:
		case OP_POST_INC_AT:
		case OP_POST_DEC_AT:
		case OP_PRE_INC_AT:
		case OP_PRE_DEC_AT:
		case OP_POST_INC_V:
		case OP_POST_DEC_V:
		case OP_PRE_INC_V:
		case OP_PRE_DEC_V:
		case OP_POST_INC_AT_V:
		case OP_POST_DEC_AT_V:
		case OP_PRE_INC_AT_V:
		case OP_PRE_DEC_AT_V:

		case OP_UNMINUS:

		case OP_NOT:
		case OP_LOG_NOT:

		case OP_POST_INC_R:
		case OP_POST_DEC_R:
		case OP_PRE_INC_R:
		case OP_PRE_DEC_R:
		case OP_POST_INC_AT_R:
		case OP_POST_DEC_AT_R:
		case OP_PRE_INC_AT_R:
		case OP_PRE_DEC_AT_R:
		case OP_POST_INC_R_V:
		case OP_POST_DEC_R_V:
		case OP_PRE_INC_R_V:
		case OP_PRE_DEC_R_V:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R_V:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R_V:

		case OP_UNMINUS_R:
			unary_operation(info, nd);
			break;


		case OP_REM_ASSIGN:
		case OP_SHL_ASSIGN:
		case OP_SHR_ASSIGN:
		case OP_AND_ASSIGN:
		case OP_XOR_ASSIGN:
		case OP_OR_ASSIGN:

		case OP_ASSIGN:
		case OP_ADD_ASSIGN:
		case OP_SUB_ASSIGN:
		case OP_MUL_ASSIGN:
		case OP_DIV_ASSIGN:

		case OP_REM_ASSIGN_AT:
		case OP_SHL_ASSIGN_AT:
		case OP_SHR_ASSIGN_AT:
		case OP_AND_ASSIGN_AT:
		case OP_XOR_ASSIGN_AT:
		case OP_OR_ASSIGN_AT:

		case OP_ASSIGN_AT:
		case OP_ADD_ASSIGN_AT:
		case OP_SUB_ASSIGN_AT:
		case OP_MUL_ASSIGN_AT:
		case OP_DIV_ASSIGN_AT:

		case OP_REM_ASSIGN_V:
		case OP_SHL_ASSIGN_V:
		case OP_SHR_ASSIGN_V:
		case OP_AND_ASSIGN_V:
		case OP_XOR_ASSIGN_V:
		case OP_OR_ASSIGN_V:

		case OP_ASSIGN_V:
		case OP_ADD_ASSIGN_V:
		case OP_SUB_ASSIGN_V:
		case OP_MUL_ASSIGN_V:
		case OP_DIV_ASSIGN_V:

		case OP_REM_ASSIGN_AT_V:
		case OP_SHL_ASSIGN_AT_V:
		case OP_SHR_ASSIGN_AT_V:
		case OP_AND_ASSIGN_AT_V:
		case OP_XOR_ASSIGN_AT_V:
		case OP_OR_ASSIGN_AT_V:

		case OP_ASSIGN_AT_V:
		case OP_ADD_ASSIGN_AT_V:
		case OP_SUB_ASSIGN_AT_V:
		case OP_MUL_ASSIGN_AT_V:
		case OP_DIV_ASSIGN_AT_V:

		case OP_REM:
		case OP_SHL:
		case OP_SHR:
		case OP_AND:
		case OP_XOR:
		case OP_OR:
		case OP_LOG_AND:
		case OP_LOG_OR:

		case OP_EQ:
		case OP_NE:
		case OP_LT:
		case OP_GT:
		case OP_LE:
		case OP_GE:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:

		case OP_ASSIGN_R:
		case OP_ADD_ASSIGN_R:
		case OP_SUB_ASSIGN_R:
		case OP_MUL_ASSIGN_R:
		case OP_DIV_ASSIGN_R:

		case OP_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R:

		case OP_ASSIGN_R_V:
		case OP_ADD_ASSIGN_R_V:
		case OP_SUB_ASSIGN_R_V:
		case OP_MUL_ASSIGN_R_V:
		case OP_DIV_ASSIGN_R_V:

		case OP_ASSIGN_AT_R_V:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_MUL_ASSIGN_AT_R_V:
		case OP_DIV_ASSIGN_AT_R_V:

		case OP_EQ_R:
		case OP_NE_R:
		case OP_LT_R:
		case OP_GT_R:
		case OP_LE_R:
		case OP_GE_R:
		case OP_ADD_R:
		case OP_SUB_R:
		case OP_MUL_R:
		case OP_DIV_R:
			binary_operation(info, nd);
			break;


		default:
			operand(info, nd);
			break;
	}

	if (node_get_type(nd) == OP_EXPR_END)
	{
		node_set_next(nd);
	}
}

static void statement(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_BLOCK:
		{
			block(info, nd);
			node_set_next(nd); // OP_BLOCK_END
		}
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
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_body;
			info->label_false = label_end;

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
		}
		break;
		case OP_DO:
		{
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;
			const item_t label_loop = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_loop;
			info->label_false = label_end;

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
			const item_t label_condition = info->label_num++;
			const item_t label_body = info->label_num++;
			const item_t label_incr = info->label_num++;
			const item_t label_end = info->label_num++;

			info->label_true = label_body;
			info->label_false = label_end;

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
		}
		break;
		case OP_LABEL:
		{
			node_set_next(nd);
			statement(info, nd);
		}
		break;
		case OP_BREAK:
		case OP_CONTINUE:
		case OP_GOTO:
			node_set_next(nd);
			break;
		case OP_RETURN_VOID:
		{
			if (info->was_dynamic)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			uni_printf(info->io, " ret void\n");
		}
		break;
		case OP_RETURN_VAL:
		{
			if (info->was_dynamic)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			info->variable_location = LREG;
			expression(info, nd);

			// TODO: добавить обработку других ответов (ALOGIC)
			if (info->answer_type == ACONST && info->answer_value_type == mode_integer)
			{
				uni_printf(info->io, " ret i32 %" PRIitem "\n", info->answer_const);
			}
			else if (info->answer_type == ACONST && info->answer_value_type == mode_float)
			{
				uni_printf(info->io, " ret double %f\n", info->answer_const_double);
			}
			else if (info->answer_type == AREG)
			{
				uni_printf(info->io, " ret ");
				type_to_io(info->io, info->answer_value_type);
				uni_printf(info->io, " %%.%" PRIitem "\n", info->answer_reg);
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
			const item_t N = node_get_arg(nd, 0);
			item_t args[128];
			item_t args_type[128];

			node_set_next(nd);
			const item_t string_length = node_get_arg(nd, 0);
			node_set_next(nd); // OP_STRING
			node_set_next(nd); // OP_EXPR_END
			for (item_t i = 0; i < N; i++)
			{
				info->variable_location = LREG;
				expression(info, nd);
				args[i] = info->answer_reg;
				args_type[i] = info->answer_value_type;
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
				uni_printf(info->io, ", ");
				type_to_io(info->io, args_type[i]);
				uni_printf(info->io, " signext %%.%" PRIitem, args[i]);
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
		case OP_ARRAY_INIT:
		case OP_STRUCT_INIT:
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
	node_set_next(nd); // OP_BLOCK
	while (node_get_type(nd) != OP_BLOCK_END)
	{
		switch (node_get_type(nd))
		{
			case OP_DECL_ARR:
			{
				node id = node_get_child(nd, node_get_amount(nd) - 1);
				if (node_get_type(&id) != OP_DECL_ID)
				{
					id = node_get_child(nd, node_get_amount(nd) - 2);
				}

				const item_t displ = node_get_arg(&id, 0);
				const item_t elem_type = node_get_arg(&id, 1);
				const size_t N = (size_t)node_get_arg(&id, 2);
				const size_t index = hash_add(&info->arrays, displ, 1 + N);
				hash_set_by_index(&info->arrays, index, IS_STATIC, 1);

				node_set_next(nd);
				for (size_t i = 1; i <= N; i++)
				{
					info->variable_location = LFREE;
					expression(info, nd);

					if (info->answer_type == ACONST)
					{
						if (!hash_get_by_index(&info->arrays, index, IS_STATIC))
						{
							system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
						}

						hash_set_by_index(&info->arrays, index, i, info->answer_const);
					}
					else // if (info->answer_type == AREG) динамический массив
					{
						if (hash_get_by_index(&info->arrays, index, IS_STATIC) && i > 1)
						{
							system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
						}

						hash_set_by_index(&info->arrays, index, i, info->answer_reg);
						hash_set_by_index(&info->arrays, index, IS_STATIC, 0);
					}
				}

				if (hash_get_by_index(&info->arrays, index, IS_STATIC))
				{
					to_code_alloc_array_static(info, index, elem_type == mode_integer ? mode_integer : mode_float);
				}
				else
				{
					if (!info->was_dynamic)
					{
						to_code_stack_save(info);
					}
					to_code_alloc_array_dynamic(info, index, elem_type == mode_integer ? mode_integer : mode_float);
					info->was_dynamic = 1;
				}
			}
			break;
			case OP_DECL_ID:
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
					else if (mode_is_struct(info->sx, elem_type))
					{
						uni_printf(info->io, " %%struct.%" PRIitem " = alloca %%struct_opt.%" PRIitem ", align 4\n"
							, elem_type, elem_type);
					}
					info->variable_location = LMEM;
					info->request_reg = displ;
				}

				node_set_next(nd);
				if (all)
				{
					init(info, nd);
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
				const item_t func_type = mode_get(info->sx, (size_t)ident_get_mode(info->sx, ref_ident) + 1);
				info->was_dynamic = 0;

				if (ident_get_prev(info->sx, ref_ident) == TK_MAIN)
				{
					uni_printf(info->io, "define i32 @main(");
				}
				else
				{
					uni_printf(info->io, "define ");
					type_to_io(info->io, func_type);
					uni_printf(info->io, " @func%zi(", ref_ident);
				}
				uni_printf(info->io, ") {\n");

				node_set_next(&root);
				block(info, &root);
				uni_printf(info->io, "}\n\n");

				was_stack_functions |= info->was_dynamic;
			}
			break;
			case OP_BLOCK_END:
				break;
			default:
				system_error(node_unexpected, node_get_type(&root));
				return -1;
		}
	}

	if (was_stack_functions)
	{
		uni_printf(info->io, "declare i8* @llvm.stacksave()\n");
		uni_printf(info->io, "declare void @llvm.stackrestore(i8*)\n");
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
	tables_and_tree("tree.txt", &(sx->identifiers), &(sx->modes), &(sx->tree));
	if (optimize_for_llvm(ws, io, sx))
	{
		return -1;
	}
	tables_and_tree("tree1.txt", &(sx->identifiers), &(sx->modes), &(sx->tree));

	information info;
	info.io = io;
	info.sx = sx;
	info.string_num = 1;
	info.register_num = 1;
	info.label_num = 1;
	info.variable_location = LREG;
	info.request_reg = 0;
	info.answer_reg = 0;

	info.arrays = hash_create(HASH_TABLE_SIZE);

	const int ret = codegen(&info);

	hash_clear(&info.arrays);
	return ret;
}
