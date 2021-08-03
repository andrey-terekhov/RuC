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
} information;


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
					// init(info, nd, displ, elem_type);
				}
			}
			break;
			case OP_NOP:
			case OP_DECL_STRUCT:
			case OP_DECL_STRUCT_END:
				node_set_next(nd);
				break;
			default:
				// statement(info, nd);
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

	info.arrays = hash_create(HASH_TABLE_SIZE);

	structs_declaration(&info);

	const int ret = codegen(&info);

	hash_clear(&info.arrays);
	return ret;
}
