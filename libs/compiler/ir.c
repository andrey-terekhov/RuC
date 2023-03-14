/*
 *  Copyright 2023 Andrey Terekhov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "ir.h"

//
// Модуль IR - реализация промежуточное представление.
// 
// Реализованы трех адресные инструкции. 
//

#include <stdlib.h>
#include <string.h>
#include "AST.h"
#include "hash.h"
#include "node_vector.h"
#include "operations.h"
#include "syntax.h"
#include "tree.h"


//
// Утилиты.
//

#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG

#define unimplemented()\
	do {\
		printf("Используется нереализованная фича (смотри %s:%d)\n", __FILE__, __LINE__);\
		system_error(node_unexpected);\
	} while(0)

#define unreachable() \
	do {\
		printf("Достигнут участок кода, который считался недосягаемым (смотри %s:%d)\n", __FILE__, __LINE__);\
		system_error(node_unexpected);\
	} while(0)

#else

inline unimplemented(const char* msg)
{
	printf("Код для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}
inline 

#endif


//
// Значения.
//

#define IR_MAX_TEMP_VALUES 8

typedef enum ir_value_kind {
	IR_VALUE_KIND_VOID,
	IR_VALUE_KIND_CONST,
	IR_VALUE_KIND_TEMP,
	IR_VALUE_KIND_LOCAL,
	IR_VALUE_KIND_GLOBAL,
	IR_VALUE_KIND_PARAM
} ir_value_kind;

typedef node ir_value;

static const item_t IR_VALUE_VOID = -1;

static ir_value create_ir_value(const node *const nd, const ir_value_kind kind, const item_t type)
{
	const ir_value value = node_add_child(nd, kind);
	node_add_arg(&value, type);
	return value;
}

static ir_value create_ir_const_value(const node *const nd, const item_t type)
{
	return create_ir_value(nd, IR_VALUE_KIND_CONST, type);
}
static ir_value create_ir_const_int(const node *const nd, const int int_)
{
	ir_value value = create_ir_const_value(nd, TYPE_INTEGER);
	node_add_arg(&value, int_);
	return value;
}
static ir_value create_ir_const_float(const node *const nd, const double double_)
{
	ir_value value = create_ir_const_value(nd, TYPE_FLOATING);
	node_add_arg(&value, double_);
	return value;
}
static ir_value create_ir_const_string(const node *const nd, const char *const string)
{
	unimplemented();
	return node_broken();
}

static ir_value create_ir_temp_value(const node *const nd, const size_t id)
{
	ir_value value = create_ir_value(nd, IR_VALUE_KIND_TEMP, TYPE_INTEGER);
	node_add_arg(&value, id);
	return value;
}

static ir_value_kind ir_value_get_kind(const ir_value *const value)
{
	return node_get_type(value);
}
static item_t ir_value_get_type(const ir_value *const value)
{
	return TYPE_INTEGER;
}

static bool ir_value_is_temp(const ir_value *const value)
{
	return ir_value_get_type(value) == IR_VALUE_KIND_TEMP;
}
static bool ir_value_is_const(const ir_value *const value)
{
	return ir_value_get_type(value) == IR_VALUE_KIND_CONST;
}

static int ir_value_get_const_int(const ir_value *const value)
{
	return node_get_arg(value, 0);
}
static float ir_value_get_const_float(const ir_value *const value)
{
	return node_get_arg_double(value, 0);
}
static char* ir_value_get_const_string(const ir_value *const value)
{
	unimplemented();
	return NULL;
}

static item_t ir_value_get_temp_id(const ir_value *const value)
{
	return node_get_arg(value, 0);
}

static item_t ir_value_save(const ir_value *const value)
{
	return (item_t) node_save(value);
}
static ir_value ir_value_load(const vector *const tree, const item_t value)
{
	return node_load(tree, value);
}


//
// Внешние символы.
//

typedef node ir_extern;

static ir_extern create_ir_extern(const node *const nd, item_t id, item_t type)
{
	ir_extern extern_ = node_add_child(nd, id);
	node_add_arg(&extern_, type);
	return extern_;
}

static item_t ir_extern_get_id(const ir_extern *const extern_)
{
	return node_get_type(extern_);
}
static item_t ir_extern_get_type(const ir_extern *const extern_)
{
	return node_get_arg(extern_, 0);
}

//
// Глобальные символы.
//

typedef node ir_global;

static ir_global create_ir_global(const node *const nd, item_t id, item_t type)
{
	ir_global global = node_add_child(nd, id);
	node_add_arg(&global, type);
	return global;
}

static item_t ir_global_get_id(const ir_global *const global)
{
	return node_get_type(global);
}
static item_t ir_global_get_type(const ir_global *const global)
{
	return node_get_arg(global, 0);
}

// Инструкции.

//
// ic <=> instruction code.
//
// %1 - первый операнд.
// %2 - второй операнд.
// %r - третий операнд (результат).
// %pc - .
// [<val>] - значение по адресу `val`.
//
// <- - операция `поместить`.
//
typedef enum ir_ic
{
	// -
	IR_IC_NOP,

	// <label>:
	IR_IC_LABEL,

	// [%2] <- %1
	IR_IC_STORE,
	// %r <- [%1]
	IR_IC_LOAD,
	// [%r] <- %1
	IR_IC_ALLOCA,

	IR_IC_PTR,

	// %r <- %1 + %2
	IR_IC_ADD,
	// %r <- %1 - %2
	IR_IC_SUB,
	// %r <- %1 * %2
	IR_IC_MUL,
	// %r <- %1 / %2
	IR_IC_DIV,

	// %r <- %1 + %2
	IR_IC_FADD,
	// %r <- %1 - %2
	IR_IC_FSUB,
	// %r <- %1 * %2
	IR_IC_FMUL,
	// %r <- %1 / %2
	IR_IC_FDIV,

	// %r <- %1 % %2
	IR_IC_MOD,

	// %pc <- %1
	IR_IC_JMP,
	// %pc <- %1 if %2 != 0
	IR_IC_JMPNZ,
	// %pc <- %1 if %2 == 0
	IR_IC_JMPZ,

	// %r <- %1 to float
	IR_IC_ITOF,
	// %r <- %1 to int
	IR_IC_FTOI,

	//
	IR_IC_SELECT,

	IR_IC_PARAM,
	IR_IC_CALL

} ir_ic;

typedef node ir_instr;

static ir_instr create_ir_instr(const node *const nd, const ir_ic ic, const item_t op1, const item_t op2, const item_t res)
{
	ir_instr instr = node_add_child(nd, ic);
	node_add_arg(&instr, op1);
	node_add_arg(&instr, op2);
	node_add_arg(&instr, res);
	return instr;
}

static ir_ic ir_instr_get_ic(const ir_instr *const instr)
{
	return node_get_type(instr);
}
static item_t ir_instr_get_op1(const ir_instr *const instr)
{
	return node_get_arg(instr, 0);
}
static item_t ir_instr_get_op2(const ir_instr *const instr)
{
	return node_get_arg(instr, 1);
}
static item_t ir_instr_get_res(const ir_instr *const instr)
{
	return node_get_arg(instr, 2);
}

// Метки.

typedef enum ir_label_kind
{
	IR_LABEL_KIND_BEGIN,
	IR_LABEL_KIND_THEN,
	IR_LABEL_KIND_ELSE,
	IR_LABEL_KIND_END,
	IR_LABEL_KIND_BEGIN_CYCLE,
	IR_LABEL_KIND_NEXT
} ir_label_kind;

typedef node ir_label;

const item_t IR_LABEL_NULL = -1;

static ir_label create_ir_label(const node *const nd, ir_label_kind kind)
{
	static item_t id = 0;

	ir_label label = node_add_child(nd, kind);
	node_add_arg(&label, ++id);
	return label;
}

static ir_label_kind ir_label_get_kind(const ir_label *const label)
{
	return node_get_type(label);
}
static item_t ir_label_get_id(const ir_label *const label)
{
	return node_get_arg(label, 0);
}

static item_t ir_label_save(const ir_label *const label)
{
	return node_save(label);
}
static ir_label ir_label_load(const vector *const tree, const item_t id)
{
	return node_load(tree, id);
}

// Функции.

typedef node ir_function;

static ir_function create_ir_function(const node *const nd, item_t id, item_t type)
{
	ir_function function = node_add_child(nd, id);
	node_add_arg(&function, type);
	return function;
}

static size_t ir_function_get_instr_count(const ir_function *const function)
{
	return node_get_amount(function);
}
static ir_instr ir_function_get_instr(const ir_function *const function, size_t number)
{
	return node_get_child(function, number);
}

static item_t ir_function_save(const ir_function *const function)
{
	return (item_t) node_save(function);
}
static ir_function ir_function_load(const vector *const tree, item_t id)
{
	ir_function function = node_load(tree, id);
	return function;
}

// Модули.

static const size_t IR_EXTERNS_SIZE = 500;
static const size_t IR_GLOBALS_SIZE = 500;
static const size_t IR_FUNCTIONS_SIZE = 5000;
static const size_t IR_VALUES_SIZE = 1000;

struct ir_module
{
	// Каждый тип юнита модуля (extern, функция, глобальная переменная)
	// представлен отдельным linked list'ом.
	//
	// Возможно есть причины использовать hashmap'ы
	vector externs;
	node externs_root;

	vector globals;
	node globals_root;

	vector functions;
	node functions_root;

	vector values;
	node values_root;

	vector labels;
	node labels_root;
};

ir_module create_ir_module()
{
	ir_module module;

	module.externs = vector_create(IR_EXTERNS_SIZE);
	module.externs_root = node_get_root(&module.externs);

	module.globals = vector_create(IR_GLOBALS_SIZE);
	module.globals_root = node_get_root(&module.globals);

	module.functions = vector_create(IR_FUNCTIONS_SIZE);
	module.functions_root = node_get_root(&module.functions);

	module.values = vector_create(IR_VALUES_SIZE);
	module.values_root = node_get_root(&module.values);

	module.labels = vector_create(IR_VALUES_SIZE);
	module.labels_root = node_get_root(&module.labels);

	return module;
}

static size_t ir_module_get_extern_count(const ir_module *const module)
{
	return node_get_amount(&module->externs_root);
}
static ir_extern ir_module_get_extern(const ir_module *const module, size_t index)
{
	return node_get_child(&module->externs_root, index);
}

static size_t ir_module_get_global_count(const ir_module *const module)
{
	return node_get_amount(&module->globals_root);
}
static ir_global ir_module_get_global(const ir_module *const module, size_t index)
{
	return node_get_child(&module->globals_root, index);
}

static size_t ir_module_get_function_count(const ir_module *const module)
{
	return node_get_amount(&module->functions_root);
}
static ir_function ir_module_get_function(const ir_module *const module, size_t index)
{
	return node_get_child(&module->functions_root, index);
}

static size_t ir_module_get_value_count(const ir_module *const module)
{
	return node_get_amount(&module->values_root);
}
static ir_value ir_module_get_value(const ir_module *const module, size_t index)
{
	return node_get_child(&module->values_root, index);
}

static size_t ir_module_get_label_count(const ir_module *const module)
{
	return node_get_amount(&module->labels_root);
}
static ir_label ir_module_get_label(const ir_module *const module, size_t index)
{
	return node_get_child(&module->labels_root, index);
}

//
// Builder.
//

struct ir_builder
{
	syntax *sx;

	ir_module* module;

	item_t temp_values[IR_MAX_TEMP_VALUES];
	bool temp_used[IR_MAX_TEMP_VALUES];

	item_t break_label;
	item_t continue_label;
	item_t function_end_label;

	hash displs;
};


ir_builder create_ir_builder(ir_module *const module, const syntax *const sx) 
{
	ir_builder builder = (ir_builder) {
		.sx = sx,
		.module = module,

		.break_label = IR_LABEL_NULL,
		.continue_label = IR_LABEL_NULL,
		.function_end_label = IR_LABEL_NULL
	};

	//memset(&builder.temp_values, 0, sizeof(&builder.temp_values));
	//memset(&builder.temp_used, 0, sizeof(&builder.temp_used));

	return builder;
}

// 
static item_t ir_displs_add(ir_builder *const builder, const size_t id, const item_t location)
{
	unimplemented();
	return IR_VALUE_VOID;
}

static item_t ir_displs_get(ir_module *const module, const size_t identifier)
{
	unimplemented();
	return IR_VALUE_VOID;
}


// Функции создания инструкций.

static void ir_build_extern(ir_builder *const builder, const item_t id, const item_t type)
{
	create_ir_extern(&builder->module->externs_root, id, type);
}

static void ir_build_global(ir_builder *const builder, const item_t id, const item_t type)
{
	create_ir_global(&builder->module->globals_root, id, type);
}

// Building values.

static item_t ir_build_const_int(ir_builder *const builder, const int int_)
{
	ir_value value = create_ir_const_int(&builder->module->values_root, int_);
	return ir_value_save(&value);
}
static item_t ir_build_const_float(ir_builder *const builder, const float float_)
{
	ir_value value = create_ir_const_float(&builder->module->values_root, float_);
	return ir_value_save(&value);
}
static item_t ir_build_const_string(ir_builder *const builder, char* string)
{
	size_t size = strlen(string);
	// Stub.
	unimplemented();
	return -1;
}
static item_t ir_build_temp(ir_builder *const builder, const item_t type)
{
	const ir_value value = create_ir_temp_value(&builder->module->values_root, type);
	const item_t value_id = ir_value_save(&value);

	size_t temp_id = 0;

	while(temp_id < IR_MAX_TEMP_VALUES && builder->temp_used[temp_id])
		temp_id++;
	if (temp_id == IR_MAX_TEMP_VALUES)
	{
		// Stub.
		unimplemented();
	}


	builder->temp_values[temp_id] = value_id;

	return value_id;
}
static void ir_free_value(ir_builder *const builder, item_t value)
{
	const ir_value temp = ir_value_load(&builder->module->values, value);
	if (!ir_value_is_temp(&temp))
		return;

	builder->temp_used[ir_value_get_temp_id(&temp)] = false;
}

static item_t ir_add_label(ir_builder *const builder, const ir_label_kind kind)
{
	const ir_label label = create_ir_label(&builder->module->labels_root, kind);
	return ir_label_save(&label);
}

static ir_value ir_get_value(const ir_builder *const builder, const item_t id)
{
	return ir_value_load(&builder->module->values, (size_t) id);
}
static ir_label ir_get_label(const ir_builder *const builder, const item_t id)
{
	return ir_label_load(&builder->module->labels, (size_t) id);
}

static ir_function ir_get_function(const ir_builder *const builder, const item_t id)
{
	return ir_function_load(&builder->module->functions, (size_t) id);
}

// .

static void ir_build_instr(ir_builder *const builder, const ir_ic ic, const item_t op1, const item_t op2, const item_t res)
{
	//ir_function function = ir_function_load(builder->function);
	//ir_instr instr = create_ir_instr(function, ic, op1, op2, res);
}

static void ir_build_nop(ir_builder *const builder)
{
	ir_build_instr(builder, IR_IC_NOP, IR_VALUE_VOID, IR_VALUE_VOID, IR_VALUE_VOID);
}

static item_t ir_build_load(ir_builder *const builder, const item_t src, const item_t type)
{
	const item_t res = ir_build_temp(builder, type);
	ir_build_instr(builder, IR_IC_LOAD, src, IR_VALUE_VOID, res);
	return res;
}
static void ir_build_store(ir_builder *const builder, const item_t src, const item_t dest)
{
	ir_build_instr(builder, IR_IC_STORE, src, dest, IR_VALUE_VOID);
}
static item_t ir_build_alloca(ir_builder *const builder, const item_t type)
{
	const syntax *const sx = builder->sx;
	const item_t res = ir_build_temp(builder, TYPE_INTEGER);
	ir_build_instr(builder, IR_IC_ALLOCA, type_size(sx, type) * 4, IR_VALUE_VOID, res);
	return res;
}

static item_t ir_build_ptr(ir_builder *const builder, const item_t base, const item_t index)
{
	unimplemented();
	return IR_VALUE_VOID;
}

static void ir_build_label(ir_builder *const builder, const item_t label)
{
	ir_build_instr(builder, IR_IC_LABEL, label, IR_VALUE_VOID, IR_VALUE_VOID);
}

static item_t ir_build_bin(ir_builder *const builder, const ir_ic ic, const item_t lhs, const item_t rhs)
{
	item_t res = ir_build_temp(builder, TYPE_INTEGER);
	ir_build_instr(builder, ic, lhs, rhs, res);
	return res;
}
static item_t ir_build_add(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_ADD, lhs, rhs);
}
static item_t ir_build_sub(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_SUB, lhs, rhs);
}
static item_t ir_build_mul(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_MUL, lhs, rhs);
}
static item_t ir_build_div(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_DIV, lhs, rhs);
}

static item_t ir_build_fbin(ir_builder *const builder, const ir_ic ic, const item_t lhs, const item_t rhs)
{
	const item_t res = ir_build_temp(builder, TYPE_FLOATING);
	ir_build_instr(builder, ic, lhs, rhs, res);
	return res;
}
static item_t ir_build_fadd(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_fbin(builder, IR_IC_FADD, lhs, rhs);
}
static item_t ir_build_fsub(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_fbin(builder, IR_IC_FSUB, lhs, rhs);
}
static item_t ir_build_fdiv(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_fbin(builder, IR_IC_FDIV, lhs, rhs);
}
static item_t ir_build_fmul(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_fbin(builder, IR_IC_FMUL, lhs, rhs);
}

static item_t ir_build_mod(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_MOD, lhs, rhs);
}

static void ir_build_jmp(ir_builder *const builder, const item_t label)
{
	ir_build_instr(builder, IR_IC_JMP, label, IR_VALUE_VOID, IR_VALUE_VOID);
}
static void ir_build_jmpnz(ir_builder *const builder, const item_t label, const item_t cond)
{
	ir_build_instr(builder, IR_IC_JMPNZ, label, cond, IR_VALUE_VOID);
}
static void ir_build_jmpz(ir_builder *const builder, const item_t label, const item_t cond)
{
	ir_build_instr(builder, IR_IC_JMPZ, label, cond, IR_VALUE_VOID);
}

static item_t ir_build_itof(ir_builder *const builder, const item_t value)
{
	const item_t res = ir_build_temp(builder, TYPE_FLOATING);
	ir_build_instr(builder, IR_IC_ITOF, value, IR_VALUE_VOID, res);
	return res;
}
static item_t ir_build_ftoi(ir_builder *const builder, const item_t value)
{
	const item_t res = ir_build_temp(builder, TYPE_INTEGER);
	ir_build_instr(builder, IR_IC_FTOI, value, IR_VALUE_VOID, res);
	return res;
}


static void ir_build_param(ir_builder *const builder, const item_t value)
{
	ir_build_instr(builder, IR_IC_PARAM, value, IR_VALUE_VOID, IR_VALUE_VOID);
}
static item_t ir_build_call(ir_builder *const builder, const item_t value)
{
	const item_t res = ir_build_temp(builder, TYPE_INTEGER);
	ir_build_instr(builder, IR_IC_CALL, value, IR_VALUE_VOID, res);
	return res;
}


//
// Отладка.
//

#define ir_dumpf(...)\
	do {\
		printf(__VA_ARGS__);\
	} while (0)

static void ir_dump_type(const ir_builder *const builder, const item_t type)
{
	const syntax *const sx = builder->sx;
	if (type_is_floating(type))
		ir_dumpf("int ");
	else if (type_is_integer(sx, type))
		ir_dumpf("float ");
	else
		ir_dumpf("<? type> ");
}

static void ir_dump_value(const ir_builder *const builder, const ir_value *const value)
{
	static size_t temp_num = 0;

	const syntax *const sx = builder->sx;

	switch (ir_value_get_kind(value))
	{
		case IR_VALUE_KIND_VOID:
		{

			break;
		}
		case IR_VALUE_KIND_CONST:
		{
			const item_t type = ir_value_get_type(value);
			if (type_is_integer(sx, type))
				ir_dumpf("%d ", ir_value_get_const_int(value));
			else if (type_is_floating(type))
				ir_dumpf("%f ", ir_value_get_const_float(value));
			else
				ir_dumpf("<? const> ");
			break;
		}
		case IR_VALUE_KIND_TEMP:
		{
			const item_t id = ir_value_get_temp_id(value);
			ir_dumpf("R%d ", id);
			break;
		}
		case IR_VALUE_KIND_GLOBAL:
		{
			unimplemented();
			break;
		}
		case IR_VALUE_KIND_LOCAL:
		{
			unimplemented();
			break;
		}
		default:
		{
			unreachable();
			break;
		}
	}
}

static void ir_dump_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	ir_ic ic = ir_instr_get_ic(instr);

	item_t op1 = ir_instr_get_op1(instr);
	ir_value op1_value = ir_get_value(builder, op1);

	item_t op2 = ir_instr_get_op2(instr);
	ir_value op2_value = ir_get_value(builder, op2);

	item_t res = ir_instr_get_res(instr);
	ir_value res_value = ir_get_value(builder, res);

	if (res != IR_VALUE_VOID)
	{
		ir_dump_value(builder, res);
		ir_dumpf("= ");
	}

	switch (ic)
	{
		case IR_IC_NOP:
			ir_dumpf("nop ");
			break;
		case IR_IC_LABEL:
			break;

		case IR_IC_STORE:
			ir_dumpf("store ");
			break;
		case IR_IC_LOAD:
			ir_dumpf("load ");
			break;
		case IR_IC_ALLOCA:
			ir_dumpf("alloca ");
			break;

		case IR_IC_PTR:
			ir_dumpf("ptr ");
			break;

		case IR_IC_ADD:
			ir_dumpf("add ");
			break;
		case IR_IC_SUB:
			ir_dumpf("sub ");
			break;
		case IR_IC_MUL:
			ir_dumpf("mul ");
			break;
		case IR_IC_DIV:
			ir_dumpf("div ");
			break;

		case IR_IC_FADD:
			ir_dumpf("fadd ");
			break;
		case IR_IC_FSUB:
			ir_dumpf("fsub ");
			break;
		case IR_IC_FMUL:
			ir_dumpf("fmul ");
			break;
		case IR_IC_FDIV:
			ir_dumpf("fdiv ");
			break;

		case IR_IC_MOD:
			ir_dumpf("mod ");
			break;

		case IR_IC_JMP:
			ir_dumpf("jmp ");
			break;
		case IR_IC_JMPZ:
			ir_dumpf("jmpz ");
			break;
		case IR_IC_JMPNZ:
			ir_dumpf("jmpnz ");
			break;

		case IR_IC_ITOF:
			ir_dumpf("itof ");
			break;
		case IR_IC_FTOI:
			ir_dumpf("ftoi ");
			break;

		default:
			unreachable();
			break;
	}

	if (op1 != IR_VALUE_VOID)
	{
		ir_dump_value(builder, &op1_value);
	}
	if (op2 != IR_VALUE_VOID)
	{
		ir_dump_value(builder, &op2_value);
	}
	ir_dumpf("\n");
}

static void ir_dump_extern(const ir_builder *const builder, const ir_extern *const extern_)
{
	unimplemented();
}
static void ir_dump_global(const ir_builder *const builder, const ir_global *const global)
{
	unimplemented();
}
static void ir_dump_function(const ir_builder *const builder, const ir_function *const function)
{
	for (size_t i = 0; i < ir_function_get_instr_count(function); i++)
	{
		const ir_instr instr = ir_function_get_instr(function, i);
		ir_dump_instr(builder, function);
	}
}


static void ir_dump_module(const ir_builder *const builder, const ir_module *const module)
{
	for (size_t i = 0; i < ir_module_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_module_get_extern(module, i);
		ir_dump_extern(builder, &extern_);
	}

	for (size_t i = 0; i < ir_module_get_global_count(module); i++)
	{
		ir_global global = ir_module_get_global(module, i);
		ir_dump_global(builder, &global);
	}

	for (size_t i = 0; i < ir_module_get_function_count(module); i++)
	{
		ir_function function = ir_module_get_function(module, i);
		ir_dump_function(builder, &function);
	}
}

//
// Генерация выражений.
//

static item_t ir_emit_expression(ir_builder *const builder, const node *const nd);
static item_t ir_emit_binary_operation(ir_builder *const builder, const item_t lhs, const item_t rhs, const binary_t operator);

static item_t ir_emit_identifier_lvalue(ir_builder* const builder, const node *const nd)
{
	unimplemented();
	return IR_VALUE_VOID;
}

static item_t ir_emit_subscript_lvalue(ir_builder* const builder, const node *const nd)
{
	const item_t type = expression_get_type(nd);

	const node base = expression_subscript_get_base(nd);
	const node index = expression_subscript_get_index(nd);

	const item_t base_value = ir_emit_expression(builder, &base);
	const item_t index_value = ir_emit_expression(builder, &index);

	const item_t res_value = ir_build_ptr(builder, base_value, index_value);

	ir_free_value(builder, base_value);
	ir_free_value(builder, index_value);

	return res_value;
}

static item_t ir_emit_member_lvalue(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;
	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);

	const bool is_arrow = expression_member_is_arrow(nd);
	const item_t struct_type = is_arrow ? type_pointer_get_element_type(builder->sx, base_type) : base_type;

	unimplemented();

	return IR_VALUE_VOID;
}

static item_t ir_emit_indirection_lvalue(ir_builder *const builder, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);
	const item_t type = expression_get_type(nd);

	const item_t base_value = ir_emit_expression(builder, &operand);
	// FIXME: грузить константу на регистр в случае константных указателей
	const item_t res_value = ir_build_ptr(builder, base_value, 0);

	ir_free_value(builder, base_value);

	return res_value;
}

static item_t ir_emit_lvalue(ir_builder *const builder, const node *const nd)
{
	assert(expression_is_lvalue(nd));

	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return ir_emit_identifier_lvalue(builder, nd);

		case EXPR_SUBSCRIPT:
			return ir_emit_subscript_lvalue(builder, nd);

		case EXPR_MEMBER:
			return ir_emit_member_lvalue(builder, nd);

		case EXPR_UNARY:  // Только UN_INDIRECTION
			return ir_emit_indirection_lvalue(builder, nd);

		default:
			// Не может быть lvalue
			system_error(node_unexpected, nd);
			// Unreachable.
			return -1;
	}
}



static item_t ir_emit_literal_expression(ir_builder* const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	item_t type = expression_get_type(nd);
	switch (type_get_class(sx, type))
	{
		case TYPE_BOOLEAN:
			return ir_build_const_int(builder, expression_literal_get_boolean(nd) ? 1 : 0);
		case TYPE_CHARACTER:
			return ir_build_const_int(builder, expression_literal_get_character(nd));
		case TYPE_INTEGER:
			return ir_build_const_int(builder, expression_literal_get_integer(nd));
		case TYPE_FLOATING:
			return ir_build_const_float(builder, expression_literal_get_floating(nd));
		case TYPE_ARRAY:
			// TODO: Implement other array types.
			return ir_build_const_string(builder, expression_literal_get_string(nd));
		default:
			return IR_VALUE_VOID;
	}
}

static item_t ir_emit_call_expression(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const node callee = expression_call_get_callee(nd);
	// Конвертируем в указатель на функцию
	// FIXME: хотим рассмотреть любой callee как указатель
	// на данный момент это не поддержано в билдере, когда будет сделано -- добавить в emit_expression()
	// и применяем функцию emit_identifier_expression (т.к. его категория в билдере будет проставлена как rvalue)

	const size_t func_ref = expression_identifier_get_id(&callee);
	const size_t params_amount = expression_call_get_arguments_amount(nd);
	const item_t return_type = type_function_get_return_type(sx, expression_get_type(&callee));

		// TODO: структуры / массивы в параметры
	item_t arg_values[params_amount];
	for (size_t i = 0; i < params_amount; i++)
	{
		const node arg = expression_call_get_argument(nd, i);
		arg_values[i] = ir_emit_expression(builder, &arg);
		//const rvalue arg_rvalue = (tmp.kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, &tmp) : tmp;
	}
	for (size_t i = 0; i < params_amount; i++)
	{
		ir_build_param(builder, arg_values[i]);
		ir_free_value(builder, arg_values[i]);
	}

	const item_t value = ir_build_call(builder, (item_t) func_ref);

	return value;
}

static item_t ir_emit_member_expression(ir_builder *const builder, const node *const nd)
{
	// FIXME: возврат структуры из функции. Указателя тут оказаться не может
	return IR_VALUE_VOID;
}

static item_t ir_emit_cast_expression(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const node operand = expression_cast_get_operand(nd);
	const item_t target_type = expression_get_type(nd);
	const item_t source_type = expression_get_type(&operand);

	const item_t value = ir_emit_expression(builder, &operand);

	if (type_is_integer(sx, source_type) && type_is_floating(target_type))
	{
		return ir_build_itof(builder, value);
	}
	else
	{
		return ir_build_ftoi(builder, value);
	}
}

static item_t ir_emit_increment_expression(ir_builder *const builder, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);
	const unary_t operator = expression_unary_get_operator(nd);
	const bool is_prefix = (operator == UN_PREDEC) || (operator == UN_PREINC);

	item_t operand_value = ir_emit_lvalue(builder, &operand);
	item_t imm_rvalue = ((operator == UN_PREINC) || (operator == UN_POSTINC)) ? 1 : -1;

	unimplemented();

	return IR_VALUE_VOID;
	if (is_prefix)
	{
		// ir_emit_binary_operation(builder, operand_rvalue, operand_rvalue, imm_rvalue, BIN_ADD);
		// ir_build_add(builder, operand_value);
		// ir_build_store(builder, operand_lvalue, operand_rvalue);
		// return operand_value;
	}
	else
	{
		// const ir_value* result_value = ir_build_temp(builder);

		// ir_emit_binary_operation(enc, &post_result_rvalue, &operand_rvalue, &imm_rvalue, BIN_ADD);
		// const item_t incremented_value = ir_build_add(builder, operand_value, imm_value, post_result_value);
		// ir_build_store(builder, operand_value, post_result_value);
		// ir_free_value(builder, incremented_value);
	}
}

static item_t ir_emit_unary_expression(ir_builder *const builder, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);
	assert(operator != UN_INDIRECTION);

	// switch (operator)
	// {
	// 	case UN_POSTINC:
	// 	case UN_POSTDEC:
	// 	case UN_PREINC:
	// 	case UN_PREDEC:
	// 		return ir_emit_increment_expression(builder, nd);

	// 	case UN_MINUS:
	// 	case UN_NOT:
	// 	{
	// 		const node operand = expression_unary_get_operand(nd);
	// 		const item_t operand_value = ir_emit_expression(builder, &operand);
	// 		const binary_t instruction = (operator == UN_MINUS) ? BIN_MUL : BIN_XOR;

	// 		ir_emit_binary_operation(builder, &operand_rvalue, &operand_rvalue, &RVALUE_NEGATIVE_ONE, instruction);
	// 		return operand_rvalue;
	// 	}

	// 	case UN_LOGNOT:
	// 	{
	// 		const node operand = expression_unary_get_operand(nd);
	// 		const rvalue value = emit_expression(enc, &operand);

	// 		return value;
	// 	}

	// 	case UN_ABS:
	// 	{
	// 		const node operand = expression_unary_get_operand(nd);
	// 		const rvalue operand_rvalue = emit_expression(enc, &operand);
	// 		const mips_instruction_t instruction = type_is_floating(operand_rvalue.type) ? IC_MIPS_ABS_S : IC_MIPS_ABS;

	// 		to_code_2R(enc->sx->io, instruction, operand_rvalue.val.reg_num, operand_rvalue.val.reg_num);
	// 		return operand_rvalue;
	// 	}

	// 	case UN_ADDRESS:
	// 	{
	// 		const node operand = expression_unary_get_operand(nd);
	// 		const lvalue operand_lvalue = emit_lvalue(enc, &operand);

	// 		assert(operand_lvalue.kind != LVALUE_KIND_REGISTER);

	// 		const rvalue result_rvalue = {
	// 			.from_lvalue = !FROM_LVALUE,
	// 			.kind = RVALUE_KIND_REGISTER,
	// 			.val.reg_num = get_register(enc),
	// 			.type = TYPE_INTEGER
	// 		};

	// 		to_code_2R_I(
	// 			enc->sx->io,
	// 			IC_MIPS_ADDI,
	// 			result_rvalue.val.reg_num,
	// 			operand_lvalue.base_reg,
	// 			operand_lvalue.loc.displ
	// 		);
	// 		return result_rvalue;
	// 	}

	// 	case UN_UPB:
	// 	{
	// 		const node operand = expression_unary_get_operand(nd);
	// 		const rvalue array_address = emit_expression(enc, &operand);
	// 		const lvalue size_lvalue = {
	// 			.base_reg = array_address.val.reg_num,
	// 			.kind = LVALUE_KIND_STACK,
	// 			.loc.displ = WORD_LENGTH,
	// 			.type = TYPE_INTEGER
	// 		};
	// 		return emit_load_of_lvalue(enc, &size_lvalue);
	// 	}

	// 	default:
	// 		system_error(node_unexpected);
	// 		return builder->value_void;
	// }
	unimplemented();
	return IR_VALUE_VOID;
}

static item_t ir_emit_binary_operation(ir_builder *const builder, const item_t lhs, const item_t rhs, const binary_t operator)
{
	assert(operator != BIN_LOG_AND);
	assert(operator != BIN_LOG_OR);

	// assert(dest->kind == RVALUE_KIND_REGISTER);
	// assert(first_operand->kind != RVALUE_KIND_VOID);
	// assert(second_operand->kind != RVALUE_KIND_VOID);

	unimplemented();
	// Stub.
	return 0;
}

static item_t ir_emit_binary_expression(ir_builder *const builder, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const node lhs = expression_binary_get_LHS(nd);
	const node rhs = expression_binary_get_RHS(nd);

	unimplemented();
	// Stub.
	return 0;
}

static item_t ir_emit_ternary_expression(ir_builder *const builder, const node *const nd)
{
	const node condition = expression_ternary_get_condition(nd);
	const node lhs = expression_ternary_get_LHS(nd);
	const node rhs = expression_ternary_get_RHS(nd);

	const item_t condition_value = ir_emit_expression(builder, &condition);
	const item_t lhs_value = ir_emit_expression(builder, &lhs);
	const item_t rhs_value = ir_emit_expression(builder, &rhs); 

	// const item_t res_value = ir_build_select(builder, lhs_value, rhs_value, condition_value);

	// ir_free_value(condition_value);
	// ir_free_value(lhs_value);
	// ir_free_value(rhs_value);
	unimplemented();

	return IR_VALUE_VOID;
}

static item_t ir_emit_struct_assignment(ir_builder *const builder, const item_t target, const node *const value)
{
	unimplemented();
	return IR_VALUE_VOID;
}

static item_t ir_emit_assignment_expression(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const node lhs = expression_assignment_get_LHS(nd);
	const item_t target = ir_emit_lvalue(builder, &lhs);

	const node rhs = expression_assignment_get_RHS(nd);
	const item_t rhs_type = expression_get_type(&rhs);

	const binary_t operator = expression_assignment_get_operator(nd);

	binary_t correct_operation;
	switch (operator)
	{
		case BIN_ADD_ASSIGN:
			correct_operation = BIN_ADD;
			break;

		case BIN_SUB_ASSIGN:
			correct_operation = BIN_SUB;
			break;

		case BIN_MUL_ASSIGN:
			correct_operation = BIN_MUL;
			break;

		case BIN_DIV_ASSIGN:
			correct_operation = BIN_DIV;
			break;

		case BIN_SHL_ASSIGN:
			correct_operation = BIN_SHL;
			break;

		case BIN_SHR_ASSIGN:
			correct_operation = BIN_SHR;
			break;

		case BIN_AND_ASSIGN:
			correct_operation = BIN_AND;
			break;

		case BIN_XOR_ASSIGN:
			correct_operation = BIN_XOR;
			break;

		case BIN_OR_ASSIGN:
			correct_operation = BIN_OR;
			break;

		default:
			system_error(node_unexpected);
			return IR_VALUE_VOID;
	}

	unimplemented();
	return IR_VALUE_VOID;
}

static item_t ir_emit_expression(ir_builder *const builder, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		const item_t value = ir_emit_lvalue(builder, nd);
		return ir_build_load(builder, value, TYPE_INTEGER);
	}

	// Иначе rvalue:
	switch (expression_get_class(nd))
	{
		case EXPR_LITERAL:
			return ir_emit_literal_expression(builder, nd);

		case EXPR_CALL:
			return ir_emit_call_expression(builder, nd);

		case EXPR_MEMBER:
			return ir_emit_member_expression(builder, nd);

		case EXPR_CAST:
			return ir_emit_cast_expression(builder, nd);

		case EXPR_UNARY:
			return ir_emit_unary_expression(builder, nd);

		case EXPR_BINARY:
			return ir_emit_binary_expression(builder, nd);

		case EXPR_ASSIGNMENT:
			return ir_emit_assignment_expression(builder, nd);

		case EXPR_TERNARY:
			return ir_emit_ternary_expression(builder, nd);

		/*
		// TODO: текущая ветка от feature, а туда inline_expression'ы пока не влили
		case EXPR_INLINE:
			return emit_inline_expression(enc, nd);
		*/

		case EXPR_INITIALIZER:
			unimplemented();
			return IR_VALUE_VOID;

		default:
			system_error(node_unexpected);
			return IR_VALUE_VOID;
	}
}



static item_t ir_emit_void_expression(ir_builder *const builder, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		ir_emit_lvalue(builder, nd); // Либо регистровая переменная, либо на стеке => ничего освобождать не надо
	}
	else
	{
		const item_t res_value = ir_emit_expression(builder, nd);
		ir_free_value(builder, res_value);
	}
	return IR_VALUE_VOID;
}


//
// Генерация объявлений.
//

static void ir_emit_statement(ir_builder *const builder, const node *const nd);

static void ir_emit_array_init(ir_builder *const builder, const node *const nd, const size_t dimension, 
	const node *const init, const item_t addr)
{
	unimplemented();
}

static item_t ir_emit_bound(ir_builder *const builder, const node *const bound, const node *const nd)
{
	unimplemented();
	return IR_VALUE_VOID;
}


static void ir_emit_array_declaration(ir_builder *const builder, const node *const nd)
{
	unimplemented();
}

static void ir_emit_structure_init(ir_builder *const builder, const item_t target, const node *const initializer)
{
	//assert(type_is_structure(builder->sx, target->type));

	unimplemented();
}

static void ir_emit_variable_declaration(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const size_t identifier = declaration_variable_get_id(nd);
	const item_t type = ident_get_type(sx, identifier);

	const item_t var_ptr_value = ir_build_alloca(builder, type);

	// displacements_add(
	// 	builder,
	// 	identifier,
	// 	var_ptr_value
	// );

	unimplemented();

	if (declaration_variable_has_initializer(nd))
	{
		const node initializer = declaration_variable_get_initializer(nd);

		// if (type_is_structure(sx, type))
		// {
		// 	const item_t struct_value = ir_emit_struct_assignment(builder, &variable, &initializer);
		// 	ir_free_value(builder, struct_value);
		// }
		// else
		// {
		const item_t value = ir_emit_expression(builder, &initializer);

		ir_build_store(builder, var_ptr_value, value);
		ir_free_value(builder, value);
		// }
	}
}

static void ir_emit_function_definition(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;
	const size_t ref_ident = declaration_function_get_id(nd);

	// ir_build_function_def(builder, item_t name, );
	unimplemented();

	const item_t func_type = ident_get_type(sx, ref_ident);
	const size_t parameter_count = type_function_get_parameter_amount(sx, func_type);

	node body = declaration_function_get_body(nd);
	ir_emit_statement(builder, &body);

	// ir_build_ret(builder, IR_VALUE_VOID);
}

static void ir_emit_declaration(ir_builder *const builder, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			ir_emit_variable_declaration(builder, nd);
			break;

		case DECL_FUNC:
			ir_emit_function_definition(builder, nd);
			break;

		default:
			// С объявлением типа ничего делать не нужно
			return;
	}
}

//
// Генерация IR из выражений
//

static void ir_emit_declaration_statement(ir_builder *const builder, const node *const nd)
{
	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		ir_emit_declaration(builder, &decl);
	}
}

static void ir_emit_case_statement(ir_builder *const builder, const node *const nd, const size_t label_num)
{
	unimplemented();
}

static void ir_emit_default_statement(ir_builder *const builder, const node *const nd, const size_t label_num)
{
	unimplemented();
}

static void ir_emit_compound_statement(ir_builder *const builder, const node *const nd)
{
	//const size_t scope_displacement = enc->scope_displ;

	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		ir_emit_statement(builder, &substmt);
	}

	// enc->max_displ = max(enc->scope_displ, enc->max_displ);
	// enc->scope_displ = scope_displacement;
}

static void ir_emit_switch_statement(ir_builder *const builder, const node *const nd)
{
	unimplemented();
}

static void ir_emit_if_statement(ir_builder *const builder, const node *const nd) 
{
	const node condition = statement_if_get_condition(nd);
	const bool has_else = statement_if_has_else_substmt(nd);

	const item_t else_label = ir_add_label(builder, IR_LABEL_KIND_ELSE);
	const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);
	const item_t value = ir_emit_expression(builder, &condition);

	ir_build_jmpz(builder, has_else ? else_label : end_label, value);
	ir_free_value(builder, value);

	const node then_substmt = statement_if_get_then_substmt(nd);
	ir_emit_statement(builder, &then_substmt);

	if (has_else)
	{
		ir_build_jmp(builder, end_label);
		ir_build_label(builder, else_label);

		const node else_substmt = statement_if_get_else_substmt(nd);
		ir_emit_statement(builder, &else_substmt);
	}

	ir_build_label(builder, end_label);
}

static void ir_emit_continue_statement(ir_builder *const builder)
{
	ir_build_jmp(builder, builder->continue_label);
}

static void ir_emit_break_statement(ir_builder *const builder)
{
	ir_build_jmp(builder, builder->break_label);
}

static void ir_emit_while_statement(ir_builder *const builder, const node *const nd)
{
	const item_t begin_label = ir_add_label(builder, IR_LABEL_KIND_BEGIN_CYCLE);
	const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

	const item_t old_continue = builder->continue_label;
	const item_t old_break = builder->break_label;

	builder->continue_label = begin_label;
	builder->break_label = end_label;

	ir_build_label(builder, begin_label);

	const node condition = statement_while_get_condition(nd);
	const item_t value = ir_emit_expression(builder, &condition);

	ir_build_jmpnz(builder, end_label, value);
	ir_free_value(builder, value);

	const node body = statement_while_get_body(nd);
	ir_emit_statement(builder, &body);

	ir_build_jmp(builder, begin_label);
	ir_build_label(builder, end_label);

	builder->continue_label = old_continue;
	builder->break_label = old_break;
}

static void ir_emit_do_statement(ir_builder *const builder, const node *const nd)
{
	const item_t begin_label = ir_add_label(builder, IR_LABEL_KIND_BEGIN_CYCLE);
	ir_build_label(builder, begin_label);

	const item_t condition_label = ir_add_label(builder, IR_LABEL_KIND_NEXT);
	const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

	const item_t old_continue = builder->continue_label;
	const item_t old_break = builder->break_label;

	builder->continue_label = condition_label;
	builder->break_label = end_label;

	const node body = statement_do_get_body(nd);
	ir_emit_statement(builder, &body);
	ir_build_label(builder, condition_label);

	const node condition = statement_do_get_condition(nd);
	const item_t value = ir_emit_expression(builder, &condition);

	ir_build_jmpnz(builder, value, begin_label);
	ir_free_value(builder, value);

	ir_build_label(builder, end_label);

	builder->continue_label = old_continue;
	builder->break_label = old_break;
}

static void ir_emit_for_statement(ir_builder *const builder, const node *const nd)
{
	//const size_t scope_displacement = enc->scope_displ;

	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		ir_emit_statement(builder, &inition);
	}

	const item_t begin_label = ir_add_label(builder, IR_LABEL_KIND_BEGIN);
	const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

	const item_t old_continue = builder->continue_label;
	const item_t old_break = builder->break_label;

	builder->continue_label = begin_label;
	builder->break_label = end_label;

	ir_build_label(builder, begin_label);

	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		const item_t value = ir_emit_expression(builder, &condition);
		ir_build_jmpnz(builder, value, end_label);
		ir_free_value(builder, value);
	}

	const node body = statement_for_get_body(nd);
	ir_emit_statement(builder, &body);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		ir_emit_statement(builder, &increment);
	}

	ir_build_jmp(builder, begin_label);
	ir_build_label(builder, end_label);

	builder->continue_label = old_continue;
	builder->break_label = old_break;

	//builder->scope_displ = scope_displacement;
}

static void ir_emit_return_statement(ir_builder *const builder, const node *const nd)
{
	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		const item_t value = ir_emit_expression(builder, &expression);

		//ir_build_ret(builder, value);
	}

	//ir_build_ret(builder, IR_VALUE_VOID);
}

static void ir_emit_statement(ir_builder *const builder, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			ir_emit_declaration_statement(builder, nd);
			break;

		case STMT_CASE:
			unimplemented();
			break;

		case STMT_DEFAULT:
			unimplemented();
			break;

		case STMT_COMPOUND:
			ir_emit_compound_statement(builder, nd);
			break;

		case STMT_EXPR:
			ir_emit_void_expression(builder, nd);
			break;

		case STMT_NULL:
			break;

		case STMT_IF:
			ir_emit_if_statement(builder, nd);
			break;

		case STMT_SWITCH:
			ir_emit_switch_statement(builder, nd);
			break;

		case STMT_WHILE:
			ir_emit_while_statement(builder, nd);
			break;

		case STMT_DO:
			ir_emit_do_statement(builder, nd);
			break;

		case STMT_FOR:
			ir_emit_for_statement(builder, nd);
			break;

		case STMT_CONTINUE:
			ir_emit_continue_statement(builder);
			break;

		case STMT_BREAK:
			ir_emit_break_statement(builder);
			break;

		case STMT_RETURN:
			ir_emit_return_statement(builder, nd);
			break;

		default:
			break;
	}
}

//
// Обход IR дерева.
//

typedef struct ir_context
{
	ir_module* module;
	ir_evals* evals;
} ir_context;

static void ir_eval_bin(ir_context *const ctx, const ir_instr *const instr)
{
	unimplemented();
}
static void ir_eval_extern(ir_context *const ctx, const ir_extern *const extern_)
{
	unimplemented();
}
static void ir_eval_global(ir_context *const ctx, const ir_extern *const extern_)
{
	unimplemented();
}
static void ir_eval_function(ir_context *const ctx, const ir_function *const function)
{
	unimplemented();
}


static void ir_eval_instr(ir_context *const ctx, const ir_instr *const instr)
{
	switch (ir_instr_get_ic(instr))
	{
		case IR_IC_NOP:
			break;

		case IR_IC_LABEL:
			//ir_eval_label(ctx, instr);
			break;

		case IR_IC_ALLOCA:
			// ir_eval_alloca(ctx, instr);
			break;

		case IR_IC_LOAD:
			// ir_eval_load(ctx, instr);
			break;
		case IR_IC_STORE:
			// ir_eval_store(ctx, instr);
			break;

		case IR_IC_ADD:
		case IR_IC_SUB:
		case IR_IC_MUL:
		case IR_IC_DIV:
			// ir_eval_bin(ctx, instr);
			break;

		case IR_IC_FADD:
		case IR_IC_FSUB:
		case IR_IC_FMUL:
		case IR_IC_FDIV:
			// ir_eval_fbin(ctx, instr);
			break;

		case IR_IC_JMP:
		case IR_IC_JMPZ:
		case IR_IC_JMPNZ:
			unimplemented();
			break;

		case IR_IC_ITOF:
		case IR_IC_FTOI:
			unimplemented();
			break;

		default:
			unreachable();
	}
}
void ir_eval_module(const ir_module *const module, ir_evals* evals)
{
	void *ctx = NULL;
	for (size_t i = 0; i < ir_module_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_module_get_extern(module, i);
		ir_eval_extern(ctx, &extern_);
	}
	for (size_t i = 0; i < ir_module_get_global_count(module); i++)
	{
		ir_global global = ir_module_get_global(module, i);
		ir_eval_global(ctx, &global);
	}
	for (size_t i = 0; i < ir_module_get_function_count(module); i++)
	{
		ir_function function = ir_module_get_function(module, i);
		ir_eval_function(ctx, &function);
	}
}

