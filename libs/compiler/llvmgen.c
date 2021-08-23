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
#include "errors.h"
#include "hash.h"
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"


#define MAX_FUNCTION_ARGS 128
#define MAX_PRINTF_ARGS 128


static const size_t HASH_TABLE_SIZE = 1024;
static const size_t IS_STATIC = 0;
static const size_t MAX_DIMENSIONS = SIZE_MAX - 2;		// Из-за OP_SLICE


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

	item_t register_num;				/**< Номер регистра */
	item_t label_num;					/**< Номер метки */
	item_t init_num;					/**< Счётчик для инициализации */

	item_t request_reg;					/**< Регистр на запрос */
	location_t variable_location;		/**< Расположение переменной */

	item_t answer_reg;					/**< Регистр с ответом */
	item_t answer_const;				/**< Константа с ответом */
	double answer_const_double;			/**< Константа с ответом типа double */
	answer_t answer_kind;				/**< Вид ответа */
	item_t answer_type;					/**< Тип значения */

	item_t label_true;					/**< Метка перехода при true */
	item_t label_false;					/**< Метка перехода при false */
	item_t label_break;					/**< Метка перехода для break */
	item_t label_continue;				/**< Метка перехода для continue */

	hash arrays;						/**< Хеш таблица с информацией о массивах:
											@с key		 - смещение массива
											@c value[0]	 - флаг статичности
											@c value[1..MAX] - границы массива */

	bool was_printf;					/**< Истина, если вызывался printf в исходном коде */
	bool was_dynamic;					/**< Истина, если в функции были динамические массивы */
	bool was_file;						/**< Истина, если была работа с файлами */
} information;


static void expression(information *const info, node *const nd);
static void block(information *const info, node *const nd);


static item_t array_get_type(information *const info, const item_t array_type)
{
	item_t type = array_type;
	while (type_is_array(info->sx, type))
	{
		type = type_array_get_element_type(info->sx, type);
	}

	return type;
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
		type_to_io(info, type_pointer_get_element_type(info->sx, type));
		uni_printf(info->sx->io, "*");
	}
	else if (type_is_file(type))
	{
		uni_printf(info->sx->io, "%%struct._IO_FILE");
		info->was_file = true;
	}
}

static void operation_to_io(universal_io *const io, const item_t operation_type, const item_t type)
{
	switch (operation_type)
	{
		case BIN_ADD_ASSIGN:
		case BIN_ADD:
			uni_printf(io, type_is_integer(type) ? "add nsw" : "fadd");
			break;

		case BIN_SUB_ASSIGN:
		case BIN_SUB:
			uni_printf(io, type_is_integer(type) ? "sub nsw" : "fsub");
			break;

		case BIN_MUL_ASSIGN:
		case BIN_MUL:
			uni_printf(io, type_is_integer(type) ? "mul nsw" : "fmul");
			break;

		case BIN_DIV_ASSIGN:
		case BIN_DIV:
			uni_printf(io, type_is_integer(type) ? "sdiv" : "fdiv");
			break;

		case BIN_REM_ASSIGN:
		case BIN_REM:
			uni_printf(io, "srem");
			break;

		case BIN_SHL_ASSIGN:
		case BIN_SHL:
			uni_printf(io, "shl");
			break;

		case BIN_SHR_ASSIGN:
		case BIN_SHR:
			uni_printf(io, "ashr");
			break;

		case BIN_AND_ASSIGN:
		case BIN_AND:
			uni_printf(io, "and");
			break;

		case BIN_XOR_ASSIGN:
		case BIN_XOR:
			uni_printf(io, "xor");
			break;

		case BIN_OR_ASSIGN:
		case BIN_OR:
			uni_printf(io, "or");
			break;

		case BIN_EQ:
			uni_printf(io, type_is_integer(type) ? "icmp eq" : "fcmp oeq");
			break;
		case BIN_NE:
			uni_printf(io, type_is_integer(type) ? "icmp ne" : "fcmp one");
			break;
		case BIN_LT:
			uni_printf(io, type_is_integer(type) ? "icmp slt" : "fcmp olt");
			break;
		case BIN_GT:
			uni_printf(io, type_is_integer(type) ? "icmp sgt" : "fcmp ogt");
			break;
		case BIN_LE:
			uni_printf(io, type_is_integer(type) ? "icmp sle" : "fcmp ole");
			break;
		case BIN_GE:
			uni_printf(io, type_is_integer(type) ? "icmp sge" : "fcmp oge");
			break;
	}
}

static void to_code_operation_reg_reg(information *const info, const item_t operation
	, const item_t fst, const item_t snd, const item_t type)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation, type);
	uni_printf(info->sx->io, " ");
	type_to_io(info, type);
	uni_printf(info->sx->io, " %%.%" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_operation_reg_const_i32(information *const info, const item_t operation
	, const item_t fst, const item_t snd)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation, TYPE_INTEGER);
	uni_printf(info->sx->io, " i32 %%.%" PRIitem ", %" PRIitem "\n", fst, snd);
}

static void to_code_operation_reg_const_double(information *const info, const item_t operation
	, const item_t fst, const double snd)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation, TYPE_FLOATING);
	uni_printf(info->sx->io, " double %%.%" PRIitem ", %f\n", fst, snd);
}

static void to_code_operation_const_reg_i32(information *const info, const item_t operation
	, const item_t fst, const item_t snd)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation, TYPE_INTEGER);
	uni_printf(info->sx->io, " i32 %" PRIitem ", %%.%" PRIitem "\n", fst, snd);
}

static void to_code_operation_const_reg_double(information *const info, const item_t operation
	, const double fst, const item_t snd)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = ", info->register_num);
	operation_to_io(info->sx->io, operation, TYPE_FLOATING);
	uni_printf(info->sx->io, " double %f, %%.%" PRIitem "\n", fst, snd);
}

static void to_code_load(information *const info, const item_t result, const item_t displ, const item_t type
	, const bool is_array)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = load ", result);
	type_to_io(info, type);
	uni_printf(info->sx->io, ", ");
	type_to_io(info, type);
	uni_printf(info->sx->io, "* %%%s.%" PRIitem ", align 4\n", is_array ? "" : "var", displ);
}

static void to_code_store_reg(information *const info, const item_t reg, const item_t displ, const item_t type
	, const bool is_array, const bool is_pointer)
{
	uni_printf(info->sx->io, " store ");
	type_to_io(info, type);
	uni_printf(info->sx->io, " %%%s.%" PRIitem ", ", is_pointer ? "var" : "", reg);
	type_to_io(info, type);
	uni_printf(info->sx->io, "* %%%s.%" PRIitem ", align 4\n", is_array ? "" : "var", displ);
}

static inline void to_code_store_const_i32(information *const info, const item_t arg, const item_t displ
	, const bool is_array)
{
	uni_printf(info->sx->io, " store i32 %" PRIitem ", i32* %%%s.%" PRIitem ", align 4\n"
		, arg, is_array ? "" : "var", displ);
}

static inline void to_code_store_const_double(information *const info, const double arg, const item_t displ
	, const bool is_array)
{
	uni_printf(info->sx->io, " store double %f, double* %%%s.%" PRIitem ", align 4\n"
		, arg, is_array ? "" : "var", displ);
}

static void to_code_try_zext_to(information *const info)
{
	if (info->answer_kind != ALOGIC)
	{
		return;
	}

	uni_printf(info->sx->io, " %%.%" PRIitem " = zext i1 %%.%" PRIitem " to i32\n", info->register_num, info->answer_reg);
	info->answer_kind = AREG;
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

static void to_code_stack_save(information *const info)
{
	// команды сохранения состояния стека
	uni_printf(info->sx->io, " %%dyn = alloca i8*, align 4\n");
	uni_printf(info->sx->io, " %%.%" PRIitem " = call i8* @llvm.stacksave()\n", info->register_num);
	uni_printf(info->sx->io, " store i8* %%.%" PRIitem ", i8** %%dyn, align 4\n", info->register_num);
	info->register_num++;
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
	if (dim == 0 || dim > MAX_DIMENSIONS)
	{
		system_error(such_array_is_not_supported);
		return;
	}

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

static void to_code_alloc_array_dynamic(information *const info, const size_t index, const item_t type)
{
	// выделение памяти на стеке
	item_t to_alloc = hash_get_by_index(&info->arrays, index, 1);

	const size_t dim = hash_get_amount_by_index(&info->arrays, index) - 1;
	if (dim == 0 || dim > MAX_DIMENSIONS)
	{
		system_error(such_array_is_not_supported);
		return;
	}

	for (size_t i = 2; i <= dim; i++)
	{
		uni_printf(info->sx->io, " %%.%" PRIitem " = mul nuw i32 %%.%" PRIitem ", %%.%" PRIitem "\n"
			, info->register_num, to_alloc, hash_get_by_index(&info->arrays, index, i));
		to_alloc = info->register_num++;
	}
	uni_printf(info->sx->io, " %%dynarr.%" PRIitem " = alloca ", hash_get_key(&info->arrays, index));
	type_to_io(info, type);
	uni_printf(info->sx->io, ", i32 %%.%" PRIitem ", align 4\n", to_alloc);
}

static void to_code_slice(information *const info, const item_t displ, const size_t cur_dimension
	, const item_t prev_slice, const item_t type)
{
	uni_printf(info->sx->io, " %%.%" PRIitem " = getelementptr inbounds ", info->register_num);
	const size_t dimensions = hash_get_amount(&info->arrays, displ) - 1;

	if (hash_get(&info->arrays, displ, IS_STATIC))
	{
		for (size_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->sx->io, "[%" PRIitem " x ", hash_get(&info->arrays, displ, i));
		}
		type_to_io(info, type);

		for (size_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->sx->io, "]");
		}
		uni_printf(info->sx->io, ", ");

		for (size_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->sx->io, "[%" PRIitem " x ", hash_get(&info->arrays, displ, i));
		}
		type_to_io(info, type);

		for (size_t i = dimensions - cur_dimension; i <= dimensions; i++)
		{
			uni_printf(info->sx->io, "]");
		}

		if (cur_dimension == dimensions - 1)
		{
			uni_printf(info->sx->io, "* %%arr.%" PRIitem ", i32 0", displ);
		}
		else
		{
			uni_printf(info->sx->io, "* %%.%" PRIitem ", i32 0", prev_slice);
		}
	}
	else if (cur_dimension == dimensions - 1)
	{
		type_to_io(info, type);
		uni_printf(info->sx->io, ", ");
		type_to_io(info, type);
		uni_printf(info->sx->io, "* %%dynarr.%" PRIitem, displ);
	}
	else
	{
		type_to_io(info, type);
		uni_printf(info->sx->io, ", ");
		type_to_io(info, type);
		uni_printf(info->sx->io, "* %%.%" PRIitem, prev_slice);
	}

	if (info->answer_kind == AREG)
	{
		uni_printf(info->sx->io, ", i32 %%.%" PRIitem "\n", info->answer_reg);
	}
	else // if (info->answer_kind == ACONST)
	{
		uni_printf(info->sx->io, ", i32 %" PRIitem "\n", info->answer_const);
	}

	info->register_num++;
}


static void to_code_try_widen(information *const info, const item_t operation_type)
{
	if (operation_type == info->answer_type)
	{
		return;
	}

	if (info->answer_kind == ACONST)
	{
		info->answer_const_double = (double)info->answer_const;
	}
	else
	{
		uni_printf(info->sx->io, " %%.%" PRIitem " = sitofp ", info->register_num);
		type_to_io(info, info->answer_type);
		uni_printf(info->sx->io, " %%.%" PRIitem " to ", info->answer_reg);
		type_to_io(info, operation_type);
		uni_printf(info->sx->io, "\n");

		info->answer_type = operation_type;
		info->answer_reg = info->register_num++;
	}
}

static void check_type_and_branch(information *const info)
{
	switch (info->answer_kind)
	{
		case ACONST:
			to_code_unconditional_branch(info, info->answer_const ? info->label_true : info->label_false);
			break;
		case AREG:
		{
			to_code_operation_reg_const_i32(info, BIN_NE, info->answer_reg, 0);
			info->answer_reg = info->register_num++;
		}
		case ALOGIC:
			to_code_conditional_branch(info);
			break;
		case AMEM:
			break;
	}
}

static void assignment_expression(information *const info, node *const nd)
{
	const binary_t assignment_type = (binary_t)node_get_arg(nd, 2);
	const item_t operation_type = node_get_arg(nd, 0);

	node_set_next(nd);
	item_t displ = 0;
	bool is_array = node_get_type(nd) != OP_IDENTIFIER;
	if (!is_array)
	{
		displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
		node_set_next(nd);
	}
	else // OP_SLICE_IDENT
	{
		info->variable_location = LMEM;
		expression(info, nd); // OP_SLICE_IDENT or UN_ADDRESS
		displ = info->answer_reg;
	}

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_widen(info, operation_type);
	to_code_try_zext_to(info);
	item_t result = info->answer_reg;

	if (assignment_type != BIN_ASSIGN)
	{
		to_code_load(info, info->register_num, displ, operation_type, is_array);
		info->register_num++;

		if (info->answer_kind == AREG)
		{
			to_code_operation_reg_reg(info, assignment_type, info->register_num - 1, info->answer_reg, operation_type);
		}
		else if (type_is_integer(operation_type)) // ACONST
		{
			to_code_operation_reg_const_i32(info, assignment_type, info->register_num - 1, info->answer_const);
		}
		else
		{
			to_code_operation_reg_const_double(info, assignment_type, info->register_num - 1
				, info->answer_const_double);
		}

		result = info->register_num++;
		info->answer_kind = AREG;
	}

	if (info->answer_kind == AREG || info->answer_kind == AMEM)
	{
		to_code_store_reg(info, result, displ, operation_type, is_array
			, info->answer_kind == AMEM);
	}
	else if (type_is_integer(operation_type)) // ACONST и опериция =
	{
		to_code_store_const_i32(info, info->answer_const, displ, is_array);
	}
	else
	{
		to_code_store_const_double(info, info->answer_const_double, displ, is_array);
	}
}

static void integral_expression(information *const info, node *const nd, const answer_t kind)
{
	const binary_t operation = (binary_t)node_get_arg(nd, 2);
	const item_t operation_type = node_get_arg(nd, 0);
	node_set_next(nd);

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_widen(info, operation_type);
	to_code_try_zext_to(info);

	const answer_t left_kind = info->answer_kind;
	const item_t left_reg = info->answer_reg;
	const item_t left_const = info->answer_const;
	const double left_const_double = info->answer_const_double;

	info->variable_location = LFREE;
	expression(info, nd);

	to_code_try_widen(info, operation_type);
	to_code_try_zext_to(info);

	const answer_t right_kind = info->answer_kind;
	const item_t right_reg = info->answer_reg;
	const item_t right_const = info->answer_const;
	const double right_const_double = info->answer_const_double;

	info->answer_type = kind != ALOGIC ? operation_type : TYPE_INTEGER;

	if (left_kind == AREG && right_kind == AREG)
	{
		to_code_operation_reg_reg(info, operation, left_reg, right_reg, operation_type);
	}
	else if (left_kind == AREG && right_kind == ACONST && type_is_integer(operation_type))
	{
		to_code_operation_reg_const_i32(info, operation, left_reg, right_const);
	}
	else if (left_kind == AREG && right_kind == ACONST) // double
	{
		to_code_operation_reg_const_double(info, operation, left_reg, right_const_double);
	}
	else if (left_kind == ACONST && right_kind == AREG && operation_type)
	{
		to_code_operation_const_reg_i32(info, operation, left_const, right_reg);
	}
	else if (left_kind == ACONST && right_kind == AREG) // double
	{
		to_code_operation_const_reg_double(info, operation, left_const_double, right_reg);
	}
	else if (left_kind == ACONST && right_kind == ACONST && type_is_integer(operation_type))
	{
		info->answer_kind = ACONST;

		switch (operation)
		{
			case BIN_ADD:
				info->answer_const = left_const + right_const;
				break;
			case BIN_SUB:
				info->answer_const = left_const - right_const;
				break;
			case BIN_MUL:
				info->answer_const = left_const * right_const;
				break;
			case BIN_DIV:
				info->answer_const = left_const / right_const;
				break;
			case BIN_REM:
				info->answer_const = left_const % right_const;
				break;
			case BIN_SHL:
				info->answer_const = left_const << right_const;
				break;
			case BIN_SHR:
				info->answer_const = left_const >> right_const;
				break;
			case BIN_AND:
				info->answer_const = left_const & right_const;
				break;
			case BIN_XOR:
				info->answer_const = left_const ^ right_const;
				break;
			case BIN_OR:
				info->answer_const = left_const | right_const;
				break;

			case BIN_EQ:
				info->answer_const = left_const == right_const;
				break;
			case BIN_NE:
				info->answer_const = left_const != right_const;
				break;
			case BIN_LT:
				info->answer_const = left_const < right_const;
				break;
			case BIN_GT:
				info->answer_const = left_const > right_const;
				break;
			case BIN_LE:
				info->answer_const = left_const <= right_const;
				break;
			case BIN_GE:
				info->answer_const = left_const >= right_const;
				break;
			default:
				break;
		}
		return;
	}
	else // double
	{
		info->answer_kind = ACONST;

		switch (operation)
		{
			case BIN_ADD:
				info->answer_const_double = left_const_double + right_const_double;
				break;
			case BIN_SUB:
				info->answer_const_double = left_const_double - right_const_double;
				break;
			case BIN_MUL:
				info->answer_const_double = left_const_double * right_const_double;
				break;
			case BIN_DIV:
				info->answer_const_double = left_const_double / right_const_double;
				break;

			case BIN_EQ:
				info->answer_const = left_const_double == right_const_double;
				break;
			case BIN_NE:
				info->answer_const = left_const_double != right_const_double;
				break;
			case BIN_LT:
				info->answer_const = left_const_double < right_const_double;
				break;
			case BIN_GT:
				info->answer_const = left_const_double > right_const_double;
				break;
			case BIN_LE:
				info->answer_const = left_const_double <= right_const_double;
				break;
			case BIN_GE:
				info->answer_const = left_const_double >= right_const_double;
				break;
			default:
				break;
		}
		return;
	}

	info->answer_reg = info->register_num++;
	info->answer_kind = kind;
}

// Обрабатываются операции инкремента/декремента и постинкремента/постдекремента
static void inc_dec_expression(information *const info, node *const nd)
{
	const unary_t operation = (unary_t)node_get_arg(nd, 2);
	const item_t operation_type = node_get_arg(nd, 0);

	node_set_next(nd);
	bool is_array = node_get_type(nd) != OP_IDENTIFIER;
	item_t displ = 0;
	if (!is_array)
	{
		displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
		node_set_next(nd);
	}
	else // OP_SLICE_IDENT
	{
		info->variable_location = LMEM;
		expression(info, nd); // OP_SLICE_IDENT
		displ = info->answer_reg;
	}

	to_code_load(info, info->register_num, displ, operation_type, is_array);
	info->answer_kind = AREG;
	info->answer_reg = info->register_num++;
	info->answer_type = operation_type;

	switch (operation)
	{
		case UN_PREINC:
		case UN_PREDEC:
			info->answer_reg = info->register_num;
		case UN_POSTINC:
		case UN_POSTDEC:
		{
			if (type_is_integer(operation_type))
			{
				to_code_operation_reg_const_i32(info, operation == UN_PREINC || operation == UN_POSTINC ? BIN_ADD : BIN_SUB
					, info->register_num - 1, 1);
			}
			else // double
			{
				to_code_operation_reg_const_double(info, operation == UN_PREINC || operation == UN_POSTINC ? BIN_ADD : BIN_SUB
					, info->register_num - 1, 1.0);
			}
		}
		break;
		default:
			break;
	}

	to_code_store_reg(info, info->register_num, displ, operation_type, is_array, false);
	info->register_num++;
}

static void unary_operation(information *const info, node *const nd)
{
	switch (node_get_arg(nd, 2))
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			inc_dec_expression(info, nd);
			break;
		case UN_MINUS:
		case UN_NOT:
		{
			const unary_t operation = (unary_t)node_get_arg(nd, 2);
			const item_t operation_type = node_get_arg(nd, 0);
			node_set_next(nd);

			info->variable_location = LREG;
			expression(info, nd);

			to_code_try_zext_to(info);

			info->answer_type = TYPE_INTEGER;
			if (operation == UN_MINUS && type_is_integer(operation_type))
			{
				to_code_operation_const_reg_i32(info, BIN_SUB, 0, info->answer_reg);
			}
			else if (operation == UN_NOT)
			{
				to_code_operation_reg_const_i32(info, BIN_XOR, info->answer_reg, -1);
			}
			else if (operation == UN_MINUS && type_is_floating(operation_type))
			{
				to_code_operation_const_reg_double(info, BIN_SUB, 0, info->answer_reg);
				info->answer_type = TYPE_FLOATING;
			}

			info->answer_kind = AREG;
			info->answer_reg = info->register_num++;
		}
		break;
		case UN_LOGNOT:
		{
			const item_t temp = info->label_true;
			info->label_true =  info->label_false;
			info->label_false = temp;

			node_set_next(nd);
			expression(info, nd);
		}
		break;
		case UN_ADDRESS:
		{
			node_set_next(nd);
			info->answer_reg = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
			info->answer_kind = AMEM;
			node_set_next(nd); // Ident
		}
		break;
		case UN_INDIRECTION:
		{
			node_set_next(nd);
			info->variable_location = info->variable_location == LMEM ? LREG : LMEM;
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
	if (operation_is_assignment((binary_t)node_get_arg(nd, 2)))
	{
		assignment_expression(info, nd);
		return;
	}

	switch (node_get_arg(nd, 2))
	{
		case BIN_ADD:
		case BIN_SUB:
		case BIN_MUL:
		case BIN_DIV:

		case BIN_REM:
		case BIN_SHL:
		case BIN_SHR:
		case BIN_AND:
		case BIN_XOR:
		case BIN_OR:
			integral_expression(info, nd, AREG);
			break;

		case BIN_EQ:
		case BIN_NE:
		case BIN_LT:
		case BIN_GT:
		case BIN_LE:
		case BIN_GE:
			integral_expression(info, nd, ALOGIC);
			break;

		// TODO: протестировать и при необходимости реализовать случай, когда && и || есть в арифметических выражениях
		case BIN_LOG_OR:
		case BIN_LOG_AND:
		{
			const item_t label_next = info->label_num++;
			const item_t old_label_true = info->label_true;
			const item_t old_label_false = info->label_false;

			if (node_get_arg(nd, 2) == BIN_LOG_OR)
			{
				info->label_false = label_next;
			}
			else // (node_get_arg(nd, 2) == OP_LOG_AND)
			{
				info->label_true = label_next;
			}

			node_set_next(nd);
			expression(info, nd);

			// TODO: сделать обработку других ответов
			// постараться использовать функцию check_type_and_branch
			if (info->answer_kind == ALOGIC)
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
		case OP_IDENTIFIER:
		{
			item_t type = node_get_arg(nd, 0);
			const item_t displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
			bool is_addr_to_val = false;

			node_set_next(nd);
			if (info->variable_location == LMEM)
			{
				to_code_load(info, info->register_num, displ, type, false);
				info->register_num++;
				info->variable_location = LREG;
				is_addr_to_val = true;
				type = type_pointer_get_element_type(info->sx, type);
			}

			to_code_load(info, info->register_num, is_addr_to_val ? info->register_num - 1 : displ, type
				, is_addr_to_val);
			info->answer_reg = info->register_num++;
			info->answer_kind = AREG;
			info->answer_type = type;
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
					to_code_store_const_i32(info, num, info->request_reg, false);
					info->answer_kind = AREG;
				}
				else
				{
					info->answer_kind = ACONST;
					info->answer_const = num;
					info->answer_type = TYPE_INTEGER;
				}
			}
			else
			{
				const double num = node_get_arg_double(nd, 2);

				if (info->variable_location == LMEM)
				{
					to_code_store_const_double(info, num, info->request_reg, false);
					info->answer_kind = AREG;
				}
				else
				{
					info->answer_kind = ACONST;
					info->answer_const_double = num;
					info->answer_type = TYPE_FLOATING;
				}
			}

			node_set_next(nd);
		}
		break;
		case OP_SLICE:
		{
			const item_t type = node_get_arg(nd, 0);
			node_set_next(nd);

			// двумерная вырезка, плохое решение, более общее решение будет, когда будут реализовываться массивы бОльшей размерности
			if (node_get_type(nd) == OP_SLICE)
			{
				node_set_next(nd);

				const item_t displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
				node_set_next(nd);

				size_t cur_dimension = hash_get_amount(&info->arrays, displ) - 2;
				const location_t location = info->variable_location;

				info->variable_location = LFREE;
				expression(info, nd);

				// TODO: пока только для динамических массивов размерности 2
				if (!hash_get(&info->arrays, displ, IS_STATIC) && cur_dimension == 1)
				{
					if (info->answer_kind == ACONST)
					{
						to_code_operation_const_reg_i32(info, BIN_MUL, info->answer_const, hash_get(&info->arrays, displ, 2));
					}
					else // if (info->answer_kind == AREG)
					{
						to_code_operation_reg_reg(info, BIN_MUL, info->answer_reg, hash_get(&info->arrays, displ, 2),
							TYPE_INTEGER);
					}

					info->answer_kind = AREG;
					info->answer_reg = info->register_num++;
				}

				if (cur_dimension != 0 && cur_dimension < MAX_DIMENSIONS)
				{
					to_code_slice(info, displ, cur_dimension, 0, type);
				}
				else
				{
					system_error(such_array_is_not_supported);
				}

				item_t prev_slice = info->register_num - 1;
				info->variable_location = LFREE;
				expression(info, nd);
				cur_dimension--;

				// Проверка, что значение cur_dimension корректное и в пределах допустимого
				// cur_dimension не определена пока что для массивов в структурах и массивов-аргументов функций
				if (cur_dimension < MAX_DIMENSIONS)
				{
					to_code_slice(info, displ, cur_dimension, prev_slice, type);
				}
				else
				{
					system_error(such_array_is_not_supported);
				}

				if (location != LMEM)
				{
					to_code_load(info, info->register_num, info->register_num - 1, type, true);
					info->register_num++;
				}

				info->answer_reg = info->register_num - 1;
				info->answer_kind = AREG;
				info->answer_type = type;
				break;
			}

			const item_t displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 2));
			node_set_next(nd);

			size_t cur_dimension = hash_get_amount(&info->arrays, displ) - 2;
			const location_t location = info->variable_location;

			info->variable_location = LFREE;
			expression(info, nd);

			// Проверка, что значение cur_dimension корректное и в пределах допустимого
			// cur_dimension не определена пока что для массивов в структурах и массивов-аргументов функций
			if (cur_dimension < MAX_DIMENSIONS)
			{
				to_code_slice(info, displ, cur_dimension, 0, type);
			}
			else
			{
				system_error(such_array_is_not_supported);
			}

			if (location != LMEM)
			{
				to_code_load(info, info->register_num, info->register_num - 1, type, true);
				info->register_num++;
			}

			info->answer_reg = info->register_num - 1;
			info->answer_kind = AREG;
			info->answer_type = type;
		}
		break;

		case OP_CALL:
		{
			item_t arguments[MAX_FUNCTION_ARGS];
			double arguments_double[MAX_FUNCTION_ARGS];
			answer_t arguments_type[MAX_FUNCTION_ARGS];
			item_t arguments_value_type[MAX_FUNCTION_ARGS];

			const item_t func_type = node_get_arg(nd, 0);
			node_set_next(nd);

			const item_t type_ref = node_get_arg(nd, 0);
			const size_t args = type_function_get_parameter_amount(info->sx, type_ref);
			if (args > MAX_FUNCTION_ARGS)
			{
				system_error(too_many_arguments);
				return;
			}

			node_set_next(nd); // OP_IDENT
			for (size_t i = 0; i < args; i++)
			{
				info->variable_location = LFREE;
				expression(info, nd);
				// TODO: сделать параметры других типов (логическое)
				arguments_type[i] = info->answer_kind;
				arguments_value_type[i] = info->answer_type;
				if (info->answer_kind == AREG)
				{
					arguments[i] = info->answer_reg;
				}
				else if (type_is_integer(info->answer_type)) // ACONST
				{
					arguments[i] = info->answer_const;
				}
				else // double
				{
					arguments_double[i] = info->answer_const_double;
				}
			}

			if (!type_is_void(func_type))
			{
				uni_printf(info->sx->io, " %%.%" PRIitem " =", info->register_num);
				info->answer_kind = AREG;
				info->answer_type = func_type;
				info->answer_reg = info->register_num++;
			}
			uni_printf(info->sx->io, " call ");
			type_to_io(info, func_type);
			uni_printf(info->sx->io, " @func%" PRIitem "(", type_ref);

			for (size_t i = 0; i < args; i++)
			{
				if (i != 0)
				{
					uni_printf(info->sx->io, ", ");
				}

				type_to_io(info, arguments_value_type[i]);
				uni_printf(info->sx->io, " signext ");
				if (arguments_type[i] == AREG)
				{
					uni_printf(info->sx->io, "%%.%" PRIitem, arguments[i]);
				}
				else if (type_is_integer(arguments_value_type[i])) // ACONST
				{
					uni_printf(info->sx->io, "%" PRIitem, arguments[i]);
				}
				else // double
				{
					uni_printf(info->sx->io, "%f", arguments_double[i]);
				}
			}
			uni_printf(info->sx->io, ")\n");
		}
		break;
		case OP_STRING:
		{
			// const size_t index = (size_t)node_get_arg(nd, 2);
			// const size_t string_length = strings_length(info->sx, index);

			// uni_printf(info->sx->io, "i8* getelementptr inbounds "
			// 	"([%zu x i8], [%zu x i8]* @.str%zu, i32 0, i32 0"
			// 	, string_length + 1
			// 	, string_length + 1
			// 	, index);
			
			info->answer_reg = -1;
			info->answer_kind = AREG;
			node_set_next(nd);
		}
			break;
		case OP_SELECT:
			node_set_next(nd);
			break;
			
		case OP_UNARY:
			unary_operation(info, nd);
			break;
		case OP_BINARY:
			binary_operation(info, nd);
			break;
		case OP_TERNARY:
			node_set_next(nd);
			break;
	}
}

static void statement(information *const info, node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_NOP:
			node_set_next(nd);
			break;
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
		case OP_RETURN:
		{
			if (info->was_dynamic)
			{
				to_code_stack_load(info);
			}

			node_set_next(nd);
			info->variable_location = LREG;
			expression(info, nd);

			// TODO: добавить обработку других ответов (ALOGIC)
			if (info->answer_kind == ACONST && type_is_integer(info->answer_type))
			{
				uni_printf(info->sx->io, " ret i32 %" PRIitem "\n", info->answer_const);
			}
			else if (info->answer_kind == ACONST && type_is_floating(info->answer_type))
			{
				uni_printf(info->sx->io, " ret double %f\n", info->answer_const_double);
			}
			else if (info->answer_kind == AREG)
			{
				uni_printf(info->sx->io, " ret ");
				type_to_io(info, info->answer_type);
				uni_printf(info->sx->io, " %%.%" PRIitem "\n", info->answer_reg);
			}
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
			const item_t N = (item_t)node_get_amount(nd) - 1;
			item_t args[MAX_PRINTF_ARGS];
			item_t args_type[MAX_PRINTF_ARGS];
			if (N > MAX_PRINTF_ARGS)
			{
				system_error(too_many_arguments);
				return;
			}

			node_set_next(nd);
			const size_t index = (size_t)node_get_arg(nd, 2);
			const size_t string_length = strings_length(info->sx, index);
			node_set_next(nd); // OP_STRING

			for (item_t i = 0; i < N; i++)
			{
				info->variable_location = LREG;
				expression(info, nd);
				args[i] = info->answer_reg;
				args_type[i] = info->answer_type;
			}

			uni_printf(info->sx->io, " %%.%" PRIitem " = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
				"([%zu x i8], [%zu x i8]* @.str%zu, i32 0, i32 0)"
				, info->register_num
				, string_length + 1
				, string_length + 1
				, index);

			info->register_num++;

			for (item_t i = 0; i < N; i++)
			{
				uni_printf(info->sx->io, ", ");
				type_to_io(info, args_type[i]);
				uni_printf(info->sx->io, " signext %%.%" PRIitem, args[i]);
			}

			uni_printf(info->sx->io, ")\n");
			info->was_printf = true;
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
		const item_t N = (item_t)node_get_amount(nd);

		const size_t index = hash_get_index(&info->arrays, displ);
		hash_set_by_index(&info->arrays, index, 1, N);

		const item_t type = array_get_type(info, elem_type);
		to_code_alloc_array_static(info, index, type);

		// TODO: тут пока инициализация константами, нужно реализовать более общий случай
		node_set_next(nd);
		for (item_t i = 0; i < N; i++)
		{
			info->variable_location = LFREE;
			expression(info, nd);
			const item_t value_int = info->answer_const;
			info->answer_const = i;
			to_code_slice(info, displ, 0, 0, type);

			if (type_is_integer(type))
			{
				to_code_store_const_i32(info, value_int, info->register_num - 1, true);
			}
			else
			{
				to_code_store_const_double(info, info->answer_const_double, info->register_num - 1, true);
			}
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
			case OP_DECL_VAR:
			{
				const item_t displ = ident_get_displ(info->sx, (size_t)node_get_arg(nd, 0));
				const item_t N = node_get_arg(nd, 1);
				const item_t all = node_get_arg(nd, 2);
				const item_t elem_type = ident_get_type(info->sx, (size_t)node_get_arg(nd, 0));

				node_set_next(nd);
				if (N == 0) // обычная переменная int a; или struct point p;
				{
					uni_printf(info->sx->io, " %%var.%" PRIitem " = alloca ", displ);
					type_to_io(info, elem_type);
					uni_printf(info->sx->io, ", align 4\n");

					info->variable_location = LMEM;
					info->request_reg = displ;
				}
				else // массив
				{
					const size_t index = hash_add(&info->arrays, displ, 1 + (size_t)N);
					hash_set_by_index(&info->arrays, index, IS_STATIC, 1);

					// получение и сохранение границ
					const bool has_bounds = node_get_type(nd) != OP_LIST;
					for (item_t j = 1; j <= N && has_bounds; j++)
					{
						info->variable_location = LFREE;
						expression(info, nd);

						if (!all)
						{
							if (info->answer_kind == ACONST)
							{
								if (!hash_get_by_index(&info->arrays, index, IS_STATIC))
								{
									system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
								}

								hash_set_by_index(&info->arrays, index, (size_t)j, info->answer_const);
							}
							else // if (info->answer_kind == AREG) динамический массив
							{
								if (hash_get_by_index(&info->arrays, index, IS_STATIC) && j > 1)
								{
									system_error(array_borders_cannot_be_static_dynamic, node_get_type(nd));
								}

								hash_set_by_index(&info->arrays, index, (size_t)j, info->answer_reg);
								hash_set_by_index(&info->arrays, index, IS_STATIC, 0);
							}
						}
					}

					if (hash_get_by_index(&info->arrays, index, IS_STATIC) && !all)
					{
						to_code_alloc_array_static(info, index, array_get_type(info, elem_type));
					}
					else if (!all) // объявление массива, если он динамический
					{
						if (!info->was_dynamic)
						{
							to_code_stack_save(info);
						}

						to_code_alloc_array_dynamic(info, index, array_get_type(info, elem_type));
						info->was_dynamic = true;
					}
				}

				if (all)
				{
					init(info, nd, displ, elem_type);
				}
			}
			break;
			case OP_NOP:
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
	bool was_stack_functions = false;
	node root = node_get_root(&info->sx->tree);

	while (true)
	{
		switch (node_get_type(&root))
		{
			case OP_FUNC_DEF:
			{
				const size_t ref_ident = (size_t)node_get_arg(&root, 0);
				const item_t func_type = ident_get_type(info->sx, ref_ident);
				const item_t ret_type = type_function_get_return_type(info->sx, func_type);
				const size_t parameters = type_function_get_parameter_amount(info->sx, func_type);
				const bool is_main = ident_get_prev(info->sx, ref_ident) == TK_MAIN;
				info->was_dynamic = false;

				uni_printf(info->sx->io, "define ");
				type_to_io(info, ret_type);
				if (is_main)
				{
					uni_printf(info->sx->io, " @main(");
				}
				else
				{
					uni_printf(info->sx->io, " @func%" PRIitem "(", ident_get_type(info->sx, ref_ident));
				}

				for (size_t i = 0; i < parameters; i++)
				{
					uni_printf(info->sx->io, i == 0 ? "" : ", ");

					const item_t param_type = type_function_get_parameter_type(info->sx, func_type, i);
					type_to_io(info, param_type);
				}
				uni_printf(info->sx->io, ") {\n");

				for (size_t i = 0; i < parameters; i++)
				{
					const item_t param_displ = ident_get_displ(info->sx, ref_ident + 4 * (i + 1));
					const item_t param_type = type_function_get_parameter_type(info->sx, func_type, i);

					uni_printf(info->sx->io, " %%var.%" PRIitem " = alloca ", param_displ);
					type_to_io(info, param_type);
					uni_printf(info->sx->io, ", align 4\n");

					uni_printf(info->sx->io, " store ");
					type_to_io(info, param_type);
					uni_printf(info->sx->io, " %%%zu, ", i);
					type_to_io(info, param_type);
					uni_printf(info->sx->io, "* %%var.%" PRIitem ", align 4\n", param_displ);
				}

				node_set_next(&root);
				block(info, &root);

				if (type_is_void(ret_type))
				{
					if (info->was_dynamic)
					{
						to_code_stack_load(info);
					}
					uni_printf(info->sx->io, " ret void\n");
				}
				uni_printf(info->sx->io, "}\n\n");

				was_stack_functions |= info->was_dynamic;
			}
			break;
			default:
			{
				if (node_set_next(&root) != 0)
				{
					if (was_stack_functions)
					{
						uni_printf(info->sx->io, "declare i8* @llvm.stacksave()\n");
						uni_printf(info->sx->io, "declare void @llvm.stackrestore(i8*)\n");
					}

					if (info->was_printf)
					{
						uni_printf(info->sx->io, "declare i32 @printf(i8*, ...)\n");
					}

					if (info->was_file)
					{
						uni_printf(info->sx->io, "%%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, "
							"%%struct._IO_marker*, %%struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, "
							"i32, [20 x i8] }");
						uni_printf(info->sx->io, "%%struct._IO_marker = type { %%struct._IO_marker*, %%struct._IO_FILE*, i32 }");
					}

					return 0;
				}
			}
		}
	}
}

static void architecture(const workspace *const ws, syntax *const sx)
{
	for (size_t i = 0; ; i++)
	{
		const char *flag = ws_get_flag(ws, i);

		if (flag == NULL || strcmp(flag, "--x86_64") == 0)
		{
			uni_printf(sx->io, "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n");
			uni_printf(sx->io, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
			return;
		}
		else if (strcmp(flag, "--mipsel") == 0)
		{
			uni_printf(sx->io, "target datalayout = \"e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64\"\n");
			uni_printf(sx->io, "target triple = \"mipsel\"\n\n");
			return;
		}
	}
}

static void structs_declaration(information *const info)
{
	const size_t types = vector_size(&info->sx->types);
	for (size_t i = BEGIN_USER_TYPE; i < types; i++)
	{
		if (type_is_structure(info->sx, (item_t)i))
		{
			uni_printf(info->sx->io, "%%struct_opt.%zu = type { ", i);

			const size_t fields = type_structure_get_member_amount(info->sx, (item_t)i);
			for (size_t j = 0; j < fields; j++)
			{
				uni_printf(info->sx->io, j == 0 ? "" : ", ");
				type_to_io(info, type_structure_get_member_type(info->sx, (item_t)i, j));
			}

			uni_printf(info->sx->io, " }\n");
		}
	}
	uni_printf(info->sx->io, " \n");
}

static void strings_declaration(information *const info)
{
	const size_t amount = strings_amount(info->sx);
	for (size_t i = 0; i < amount; i++)
	{
		const char *string = string_get(info->sx, i);
		const size_t length = strings_length(info->sx, i);
		uni_printf(info->sx->io, "@.str%zu = private unnamed_addr constant [%zu x i8] c\""
			, i, length + 1);

		for (size_t j = 0; j < length; j++)
		{
			const char ch = string[j];
			if (ch == '\n')
			{
				uni_printf(info->sx->io, "\\0A");
			}
			else
			{
				uni_printf(info->sx->io, "%c", ch);
			}
		}
		uni_printf(info->sx->io, "\\00\", align 1\n");
	}
	uni_printf(info->sx->io, " \n");
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_llvm(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}
	// tables_and_tree("tree.txt", &(sx->identifiers), &(sx->types), &(sx->tree));

	information info;
	info.sx = sx;
	info.register_num = 1;
	info.label_num = 1;
	info.init_num = 1;
	info.variable_location = LREG;
	info.request_reg = 0;
	info.answer_reg = 0;
	info.was_printf = false;
	info.was_dynamic = false;
	info.was_file = false;

	info.arrays = hash_create(HASH_TABLE_SIZE);

	architecture(ws, sx);
	structs_declaration(&info);
	strings_declaration(&info);

	const int ret = codegen(&info);

	hash_clear(&info.arrays);
	return ret;
}
