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
		printf("\nИспользуется нереализованная фича (смотри %s:%d)\n", __FILE__, __LINE__);\
	} while(0)

#define unreachable() \
	do {\
		printf("\nДостигнут участок кода, который считался недосягаемым (смотри %s:%d)\n", __FILE__, __LINE__);\
	} while(0)

#else

inline unimplemented(const char* msg)
{
	printf("\nКод для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}
inline unreachable(const char* msg)
{
	printf("\nКод для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}

#endif

//
// Значения для передачи в gen_* функции.
//

// Rvalue.

rvalue create_temp_rvalue(const item_t id)
{
	return (rvalue) {
		.kind = RVALUE_KIND_TEMP,
		.id = id
	};
}
rvalue create_imm_int_rvalue(const int value)
{
	return (rvalue) {
		.kind = RVALUE_KIND_IMM,
		.value.int_ = value
	};
}
rvalue create_imm_float_rvalue(const float value)
{
	return (rvalue) {
		.kind = RVALUE_KIND_IMM,
		.value.float_ = value
	};
}
rvalue create_imm_string_rvalue(const char* value)
{
	return (rvalue) {
		.kind = RVALUE_KIND_IMM,
		.value.string = value
	};
}

rvalue_kind rvalue_get_kind(const rvalue *const value)
{
	return value->kind;
}
item_t rvalue_get_type(const rvalue *const value)
{
	return value->type;
}

item_t temp_rvalue_get_id(const rvalue *const value)
{
	return value->id;
}
int imm_int_rvalue_get_value(const rvalue *const value)
{
	return value->value.int_;
}
float imm_float_rvalue_get_value(const rvalue *const value)
{
	return value->value.float_;
}
char* imm_string_rvalue_get_value(const rvalue *const value)
{
	return value->value.string;
}


// Lvalue.

lvalue create_local_lvalue(const item_t displ)
{
	return (lvalue) {
		.kind = LVALUE_KIND_LOCAL,
		.displ = displ
	};
}
lvalue create_param_lvalue(const size_t num)
{
	return (lvalue) {
		.kind = LVALUE_KIND_LOCAL,
		.num = num
	};
}
lvalue create_global_lvalue(const item_t displ)
{
	return (lvalue) {
		.kind = LVALUE_KIND_LOCAL,
		.displ = displ
	};
}

lvalue_kind lvalue_get_kind(const lvalue *const value)
{
	return value->kind;
}
item_t lvalue_get_type(const lvalue *const value)
{
	return value->type;
}

item_t local_lvalue_get_displ(const lvalue *const value)
{
	return value->displ;
}
size_t param_lvalue_get_num(const lvalue *const value)
{
	return value->num;
}
item_t global_lvalue_get_displ(const lvalue *const value)
{
	return value->displ;
}


//
// Значения.
//

typedef enum ir_value_kind {
	IR_VALUE_KIND_VOID,
	IR_VALUE_KIND_IMM,
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

static ir_value create_ir_imm_value(const node *const nd, const item_t type)
{
	return create_ir_value(nd, IR_VALUE_KIND_IMM, type);
}
static ir_value create_ir_imm_int(const node *const nd, const int int_)
{
	ir_value value = create_ir_imm_value(nd, TYPE_INTEGER);
	node_add_arg(&value, int_);
	return value;
}
static ir_value create_ir_imm_float(const node *const nd, const double double_)
{
	ir_value value = create_ir_imm_value(nd, TYPE_FLOATING);
	node_add_arg(&value, double_);
	return value;
}
static ir_value create_ir_imm_string(const node *const nd, const char *const string)
{
	unimplemented();
	return node_broken();
}

static ir_value create_ir_temp_value(const node *const nd, const item_t type, const size_t id)
{
	ir_value value = create_ir_value(nd, IR_VALUE_KIND_TEMP, type);
	node_add_arg(&value, id);
	return value;
}

static ir_value create_ir_local_value(const node *const nd, const item_t type, const size_t displ)
{
	ir_value value = create_ir_value(nd, IR_VALUE_KIND_LOCAL, type);
	node_add_arg(&value, displ);
	return value;
}

static ir_value create_ir_param_value(const node *const nd, const item_t type, const size_t num)
{
	ir_value value = create_ir_value(nd, IR_VALUE_KIND_PARAM, type);
	node_add_arg(&value, num);
	return value;
}

static ir_value_kind ir_value_get_kind(const ir_value *const value)
{
	return node_get_type(value);
}

static bool ir_value_is_temp(const ir_value *const value)
{
	return ir_value_get_kind(value) == IR_VALUE_KIND_TEMP;
}
static bool ir_value_is_imm(const ir_value *const value)
{
	return ir_value_get_kind(value) == IR_VALUE_KIND_IMM;
}

static item_t ir_value_get_type(const ir_value *const value)
{
	return node_get_arg(value, 0);
}

static int ir_imm_value_get_int(const ir_value *const value)
{
	return node_get_arg(value, 1);
}
static float ir_imm_value_get_float(const ir_value *const value)
{
	return node_get_arg_double(value, 1);
}
static char* ir_imm_value_get_string(const ir_value *const value)
{
	unimplemented();
	return NULL;
}

static item_t ir_temp_value_get_id(const ir_value *const value)
{
	return node_get_arg(value, 1);
}

static item_t ir_local_value_get_dipsl(const ir_value *const value)
{
	return node_get_arg(value, 1);
}

static size_t ir_param_value_get_num(const ir_value *const value)
{
	return node_get_arg(value, 1);
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

static ir_global create_ir_global(const node *const nd, const item_t id, const item_t type)
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

//
// Инструкции.
//

//
// ic <=> instruction code.
//
// %1 - первый операнд.
// %2 - второй операнд.
// %r (%3) - результат (третий операнд).
// %pc - счетчик программы.
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

	// %2 <- %1
	IR_IC_MOVE,

	// [%2] <- %1
	IR_IC_STORE,
	// %r <- [%1]
	IR_IC_LOAD,
	// [%r] <- %1
	IR_IC_ALLOCA,

	// %r <- %1 + %2
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

	// %r <- %1 & %2
	IR_IC_AND,
	// %r <- %1 ^ %2
	IR_IC_XOR,
	// %r <- %1 | %2
	IR_IC_OR,

	// %r <- %1 << %2
	IR_IC_SHL,
	// %r <- %1 >> %2
	IR_IC_SHR,

	// %r <- %1 % %2
	IR_IC_MOD,

	// %pc <- %1
	IR_IC_JMP,
	// %pc <- %1 if %2 != 0
	IR_IC_JMPNZ,
	// %pc <- %1 if %2 == 0
	IR_IC_JMPZ,
	// %pc <- %1 if %2 == %r
	IR_IC_JMPEQ,
	// %pc <- %1 if %2 < %r
	IR_IC_JMPLT,
	// %pc <- %1 if %2 <= %r
	IR_IC_JMPLE,


	// %r <- %1 to float
	IR_IC_ITOF,
	// %r <- %1 to int
	IR_IC_FTOI,


	// %r <- %1 < %2
	IR_IC_SLT,

	// 
	IR_IC_PARAM,
	// %pc <- %1
	IR_IC_CALL,

	// %pc <- %ra
	IR_IC_RET

} ir_ic;

typedef enum ir_instr_kind
{
	/** Format: instr */
	IR_INSTR_KIND_N,
	/** Format: instr rvalue */
	IR_INSTR_KIND_RN,
	/** Format: rvalue <- instr rvalue */
	IR_INSTR_KIND_RR,
	/** Format: instr rvalue, rvalue */
	IR_INSTR_KIND_RRN,
	/** Format: rvalue <- instr rvalue, rvalue */
	IR_INSTR_KIND_RRR,
	/** Format: lvalue <- instr rvalue */
	IR_INSTR_KIND_LR,
	/** Format: lvalue <- instr rvalue */
	IR_INSTR_KIND_RL,
	/** Format: instr rvalue, lvalue */
	IR_INSTR_KIND_RLN,
	/** Format: lvalue <- instr rvalue, size */
	IR_INSTR_KIND_SL,
	/** Format: instr label */
	IR_INSTR_KIND_BN,
	/** Format: instr label, rvalue */
	IR_INSTR_KIND_BRN,
	/** Format: instr label, rvalue, rvalue */
	IR_INSTR_KIND_BRRN,
	/** Format: instr function */
	IR_INSTR_KIND_FN,
	/** Format: rvalue <- instr function */
	IR_INSTR_KIND_FR,
} ir_instr_kind;

static ir_instr_kind ir_instr_kind_from_ic(const ir_ic ic)
{
	switch(ic)
	{
		case IR_IC_NOP:
			return IR_INSTR_KIND_N;

		case IR_IC_LABEL:
			return IR_INSTR_KIND_BN;

		case IR_IC_MOVE:
			return IR_INSTR_KIND_RRN;

		case IR_IC_STORE:
			return IR_INSTR_KIND_RLN;
		case IR_IC_LOAD:
			return IR_INSTR_KIND_LR;
		case IR_IC_ALLOCA:
			return IR_INSTR_KIND_SL;

		case IR_IC_PTR:
			return IR_INSTR_KIND_RR;

		case IR_IC_ADD:
		case IR_IC_SUB:
		case IR_IC_MUL:
		case IR_IC_DIV:
		case IR_IC_FADD:
		case IR_IC_FSUB:
		case IR_IC_FMUL:
		case IR_IC_FDIV:
		case IR_IC_AND:
		case IR_IC_XOR:
		case IR_IC_OR:
		case IR_IC_SHL:
		case IR_IC_SHR:
		case IR_IC_MOD:
			return IR_INSTR_KIND_RRR;

		case IR_IC_JMP:
			return IR_INSTR_KIND_BN;
		case IR_IC_JMPNZ:
		case IR_IC_JMPZ:
			return IR_INSTR_KIND_BRN;

		case IR_IC_JMPEQ:
		case IR_IC_JMPLT:
		case IR_IC_JMPLE:
			return IR_INSTR_KIND_BRRN;

		case IR_IC_ITOF:
		case IR_IC_FTOI:
			return IR_INSTR_KIND_RR;

		case IR_IC_SLT:
			unimplemented();
			return IR_INSTR_KIND_N;

		case IR_IC_PARAM:
			return IR_INSTR_KIND_RN;
		case IR_IC_CALL:
			return IR_INSTR_KIND_FR;

		case IR_IC_RET:
			return IR_INSTR_KIND_RN;
		default:
			unreachable();
			return IR_INSTR_KIND_N;
	}
}

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
static ir_instr_kind ir_instr_get_kind(const ir_instr *const instr)
{
	return ir_instr_kind_from_ic(ir_instr_get_ic(instr));
}
static item_t ir_instr_get_op1(const ir_instr *const instr)
{
	return node_get_arg(instr, 0);
}
static item_t ir_instr_get_op2(const ir_instr *const instr)
{
	return node_get_arg(instr, 1);
}
static item_t ir_instr_get_op3(const ir_instr *const instr)
{
	return node_get_arg(instr, 2);
}
static item_t ir_instr_get_res(const ir_instr *const instr)
{
	return ir_instr_get_op3(instr);
}

//
// Метки.
//

typedef enum ir_label_kind
{
	IR_LABEL_KIND_BEGIN,
	IR_LABEL_KIND_THEN,
	IR_LABEL_KIND_ELSE,
	IR_LABEL_KIND_END,
	IR_LABEL_KIND_BEGIN_CYCLE,
	IR_LABEL_KIND_NEXT,
	IR_LABEL_KIND_AND,
	IR_LABEL_KIND_OR
} ir_label_kind;

typedef node ir_label;

const item_t IR_LABEL_NULL = -1;

static ir_label create_ir_label(const node *const nd, ir_label_kind kind)
{
	static item_t id = 0;

	ir_label label_ = node_add_child(nd, kind);
	node_add_arg(&label_, ++id);
	return label_;
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

//
// Функции.
//

typedef node ir_function;

static ir_function create_ir_function(const node *const nd, const item_t id, const item_t type)
{
	ir_function function = node_add_child(nd, id);
	node_add_arg(&function, type);
	node_add_arg(&function, 0);
	return function;
}

static size_t ir_function_get_instr_count(const ir_function *const function)
{
	return node_get_amount(function);
}
static ir_instr ir_function_index_instr(const ir_function *const function, size_t idx)
{
	return node_get_child(function, idx);
}

static item_t ir_function_get_id(const ir_function *const function)
{
	return node_get_type(function);
}
static item_t ir_function_get_type(const ir_function *const function)
{
	return node_get_arg(function, 0);
}
static void ir_function_add_displ(ir_function *const function, const size_t displ)
{

}
static size_t ir_function_get_displ(const ir_function *const function)
{
	return node_get_arg(function, 1);
}

static item_t ir_function_save(const ir_function *const function)
{
	return (item_t) node_save(function);
}
static ir_function ir_function_load(const vector *const tree, const item_t id)
{
	ir_function function = node_load(tree, id);
	return function;
}

//
// Модули.
//

static const size_t IR_EXTERNS_SIZE = 500;
static const size_t IR_GLOBALS_SIZE = 500;
static const size_t IR_FUNCTIONS_SIZE = 5000;
static const size_t IR_VALUES_SIZE = 1000;
static const size_t IR_IDENTS_SIZE = 1000;

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

	module.idents = hash_create(IR_IDENTS_SIZE);

	return module;
}

static size_t ir_module_get_extern_count(const ir_module *const module)
{
	return node_get_amount(&module->externs_root);
}
static ir_extern ir_module_index_extern(const ir_module *const module, size_t idx)
{
	return node_get_child(&module->externs_root, idx);
}

static size_t ir_module_get_global_count(const ir_module *const module)
{
	return node_get_amount(&module->globals_root);
}
static ir_global ir_module_index_global(const ir_module *const module, size_t idx)
{
	return node_get_child(&module->globals_root, idx);
}

static size_t ir_module_get_function_count(const ir_module *const module)
{
	return node_get_amount(&module->functions_root);
}
static ir_function ir_module_index_function(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->functions_root, idx);
}
static ir_function ir_module_get_function(const ir_module *const module, const item_t id)
{
	return ir_function_load(&module->functions, id);
}

static size_t ir_module_get_value_count(const ir_module *const module)
{
	return node_get_amount(&module->values_root);
}
static ir_value ir_module_index_value(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->values_root, idx);
}
static ir_value ir_module_get_value(const ir_module *const module, const item_t id)
{
	return ir_value_load(&module->values, id);
}

static size_t ir_module_get_label_count(const ir_module *const module)
{
	return node_get_amount(&module->labels_root);
}
static ir_label ir_module_index_label(const ir_module *const module, const size_t idx)
{
	return node_get_child(&module->labels_root, idx);
}
static ir_label ir_module_get_label(const ir_module *const module, const item_t id)
{
	return ir_label_load(&module->labels, id);
}

static void ir_idents_add(ir_module *const module, const item_t id, const item_t value)
{
	const item_t idx = hash_add(&module->idents, id, 1);
	hash_set_by_index(&module->idents, idx, 0, value);
}
static item_t ir_idents_get(const ir_module *const module, const item_t id)
{
	return hash_get(&module->idents, id, 0);
}

//
// IR построитель.
//

static item_t ir_build_imm_int(ir_builder *const builder, const int int_);
static item_t ir_build_imm_float(ir_builder *const builder, const float float_);

ir_builder create_ir_builder(ir_module *const module, const syntax *const sx) 
{

	ir_builder builder = (ir_builder) {
		.sx = sx
	};

	builder.displ = 0;

	builder.module = module;

	builder.break_label = IR_LABEL_NULL;
	builder.continue_label = IR_LABEL_NULL;
	builder.function_end_label = IR_LABEL_NULL;

	return builder;
}

static void ir_displ_add(ir_builder *const builder, item_t type)
{
	const syntax *const sx = builder->sx;
	builder->displ += type_size(sx, type);
}

static void ir_displ_reset(ir_builder *const builder)
{
	builder->displ = 0;
}
static size_t ir_displ_get(ir_builder *const builder)
{
	return builder->displ;
}


//
// Функции построителя.
//

static void ir_build_extern(ir_builder *const builder, const item_t id, const item_t type)
{
	create_ir_extern(&builder->module->externs_root, id, type);
}

static void ir_build_global(ir_builder *const builder, const item_t id, const item_t type)
{
	create_ir_global(&builder->module->globals_root, id, type);
}

//
// Построение значений.
//

// FIXME: ir_build_move необходим для реализации Immediate значений как rvalue,
// поскольку далеко не все целевые архитектуры имеют функции поддерживающие
// immediate как операнд, мы передаем в почти все gen_*() инструкции temp rvalue.
static void ir_build_move(ir_builder *const builder, const item_t src, const item_t dest);

static item_t ir_build_temp(ir_builder *const builder, const item_t type)
{
	size_t temp_id = 0;

	while(temp_id < IR_MAX_TEMP_VALUES && builder->temp_used[temp_id])
		temp_id++;
	if (temp_id == IR_MAX_TEMP_VALUES)
	{
		// Stub.
		unimplemented();
	}

	const ir_value value = create_ir_temp_value(&builder->module->values_root, type, temp_id);
	const item_t value_id = ir_value_save(&value);
	builder->temp_values[temp_id] = value_id;
	builder->temp_used[temp_id] = true;

	return value_id;
}
static item_t ir_build_imm_int(ir_builder *const builder, const int int_)
{
	ir_value value = create_ir_imm_int(&builder->module->values_root, int_);
	item_t id = ir_value_save(&value);
	item_t temp_value = ir_build_temp(builder, TYPE_INTEGER);
	ir_build_move(builder, id, temp_value);
	return temp_value;
}
static item_t ir_build_imm_float(ir_builder *const builder, const float float_)
{
	ir_value value = create_ir_imm_float(&builder->module->values_root, float_);
	item_t id = ir_value_save(&value);
	item_t temp_value = ir_build_temp(builder, TYPE_FLOATING);
	ir_build_move(builder, id, temp_value);
	return temp_value;
}
static item_t ir_build_imm_zero(ir_builder *const builder)
{
	return ir_build_imm_int(builder, 0);
}
static item_t ir_build_imm_one(ir_builder *const builder)
{
	return ir_build_imm_int(builder, 1);
}
static item_t ir_build_imm_minus_one(ir_builder *const builder)
{
	return ir_build_imm_int(builder, -1);

}
static item_t ir_build_imm_fzero(ir_builder *const builder)
{
	return ir_build_imm_float(builder, 0.0);
}
static item_t ir_build_imm_fone(ir_builder *const builder)
{
	return ir_build_imm_float(builder, 1.0);
}
static item_t ir_build_imm_fminus_one(ir_builder *const builder)
{
	return ir_build_imm_float(builder, -1.0);
}
static item_t ir_build_imm_string(ir_builder *const builder, char* string)
{
	size_t size = strlen(string);
	// Stub.
	unimplemented();
	return -1;
}

static item_t ir_build_local(ir_builder *const builder, const item_t type)
{
	const syntax *const sx = builder->sx;
	const ir_value value = create_ir_local_value(&builder->module->values_root, type, ir_displ_get(builder));
	ir_displ_add(builder, type);

	return ir_value_save(&value);
}
static void ir_free_value(ir_builder *const builder, item_t value)
{
	const ir_value temp = ir_value_load(&builder->module->values, value);
	if (!ir_value_is_temp(&temp))
		return;

	builder->temp_used[ir_temp_value_get_id(&temp)] = false;
}

static item_t ir_add_label(ir_builder *const builder, const ir_label_kind kind)
{
	const ir_label label_ = create_ir_label(&builder->module->labels_root, kind);
	return ir_label_save(&label_);
}

static ir_value ir_get_value(const ir_builder *const builder, const item_t id)
{
	return ir_module_get_value(builder->module, id);
}
static ir_label ir_get_label(const ir_builder *const builder, const item_t id)
{
	return ir_module_get_label(builder->module, id);
}

static ir_function ir_get_function(const ir_builder *const builder, const item_t id)
{
	return ir_module_get_function(builder->module, id);
}
static ir_function ir_get_current_function(const ir_builder *const builder)
{
	return ir_get_function(builder, builder->function);
}

//
// Функции построения инструкций.
//

static void ir_build_instr(ir_builder *const builder, const ir_ic ic, const item_t op1, const item_t op2, const item_t res)
{
	ir_function function = ir_get_current_function(builder);
	ir_instr instr = create_ir_instr(&function, ic, op1, op2, res);
}

static void ir_build_nop(ir_builder *const builder)
{
	ir_build_instr(builder, IR_IC_NOP, IR_VALUE_VOID, IR_VALUE_VOID, IR_VALUE_VOID);
}

static void ir_build_move(ir_builder *const builder, const item_t src, const item_t dest)
{
	ir_build_instr(builder, IR_IC_MOVE, src, dest, IR_VALUE_VOID);
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

static void ir_add_displ(ir_builder *const builder, const size_t displ)
{
	ir_function function = ir_get_current_function(builder);
	ir_function_add_displ(&function, displ);
}
static item_t ir_build_alloca(ir_builder *const builder, const item_t type)
{
	const syntax *const sx = builder->sx;

	const item_t res = ir_build_local(builder, type);
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

static item_t ir_build_and(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_MOD, lhs, rhs);
}
static item_t ir_build_or(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_MOD, lhs, rhs);
}
static item_t ir_build_xor(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_MOD, lhs, rhs);
}
static item_t ir_build_shl(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_SHL, lhs, rhs);
}
static item_t ir_build_shr(ir_builder *const builder, const item_t lhs, const item_t rhs)
{
	return ir_build_bin(builder, IR_IC_SHR, lhs, rhs);
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
static void ir_build_jmpeq(ir_builder *const builder, const item_t label, const item_t lhs, const item_t rhs)
{
	ir_build_instr(builder, IR_IC_JMPEQ, label, lhs, rhs);
}
static void ir_build_jmplt(ir_builder *const builder, const item_t label, const item_t lhs, const item_t rhs)
{
	ir_build_instr(builder, IR_IC_JMPLT, label, lhs, rhs);
}
static void ir_build_jmple(ir_builder *const builder, const item_t label, const item_t lhs, const item_t rhs)
{
	ir_build_instr(builder, IR_IC_JMPLE, label, lhs, rhs);
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
static void ir_build_ret(ir_builder *const builder, const item_t value)
{
	ir_build_instr(builder, IR_IC_RET, value, IR_VALUE_VOID, IR_VALUE_VOID);
}

static void ir_build_function_definition(ir_builder *const builder, const item_t id, const item_t type)
{
	ir_function function = create_ir_function(&builder->module->functions_root, id, type);
	builder->function = ir_function_save(&function);
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
	if (type_is_integer(sx, type))
		ir_dumpf("int");
	else if (type_is_floating(type))
		ir_dumpf("float");
	else if (type_is_void(type))
		ir_dumpf("void");
	else if (type_is_pointer(sx, type))
	{
		const item_t elem_type = type_pointer_get_element_type(sx, type);
		ir_dumpf("*");
		ir_dump_type(builder, elem_type);
	}
	else if (type_is_array(sx, type))
	{
		const item_t elem_type = type_array_get_element_type(sx, type);
		ir_dumpf("[]");
		ir_dump_type(builder, elem_type);
	}
	else if (type_is_function(sx, type))
	{
		const item_t ret_type = type_function_get_return_type(sx, type);
		const size_t param_count = type_function_get_parameter_amount(sx, type);

		ir_dump_type(builder, ret_type);

		ir_dumpf("(");
		for(size_t i = 0; i < param_count; i++)
		{
			const item_t param_type = type_function_get_parameter_type(sx, type, i); 
			ir_dump_type(builder, param_type);
		}
		ir_dumpf(")");

	}
}

static void ir_dump_value(const ir_builder *const builder, const ir_value *const value)
{
	static size_t temp_num = 0;

	const syntax *const sx = builder->sx;
	const ir_value_kind kind = ir_value_get_kind(value);

	switch (kind)
	{
		case IR_VALUE_KIND_VOID:
		{

			break;
		}
		case IR_VALUE_KIND_IMM:
		{
			const item_t type = ir_value_get_type(value);
			if (type_is_integer(sx, type))
				ir_dumpf("%d", ir_imm_value_get_int(value));
			else if (type_is_floating(type))
				ir_dumpf("%f", ir_imm_value_get_float(value));
			else
				ir_dumpf("<? imm>");
			break;
		}
		case IR_VALUE_KIND_TEMP:
		{
			const item_t id = ir_temp_value_get_id(value);
			ir_dumpf("%%%" PRIitem, id);
			break;
		}
		case IR_VALUE_KIND_GLOBAL:
		{
			unimplemented();
			break;
		}
		case IR_VALUE_KIND_LOCAL:
		{
			ir_dumpf("(%lld)", ir_local_value_get_dipsl(value));
			break;
		}
		default:
		{
			unreachable();
			break;
		}
	}
}
static void ir_dump_ic(const ir_builder *const builder, const ir_ic ic)
{
	(void) builder;

	switch (ic)
	{
		case IR_IC_NOP:
			ir_dumpf("nop");
			break;

		case IR_IC_LABEL:
			ir_dumpf("label");
			break;

		case IR_IC_MOVE:
			ir_dumpf("move");
			break;

		case IR_IC_STORE:
			ir_dumpf("store");
			break;
		case IR_IC_LOAD:
			ir_dumpf("load");
			break;
		case IR_IC_ALLOCA:
			ir_dumpf("alloca");
			break;

		case IR_IC_PTR:
			ir_dumpf("ptr");
			break;

		case IR_IC_ADD:
			ir_dumpf("add");
			break;
		case IR_IC_SUB:
			ir_dumpf("sub");
			break;
		case IR_IC_MUL:
			ir_dumpf("mul");
			break;
		case IR_IC_DIV:
			ir_dumpf("div");
			break;

		case IR_IC_FADD:
			ir_dumpf("fadd");
			break;
		case IR_IC_FSUB:
			ir_dumpf("fsub");
			break;
		case IR_IC_FMUL:
			ir_dumpf("fmul");
			break;
		case IR_IC_FDIV:
			ir_dumpf("fdiv");
			break;

		case IR_IC_MOD:
			ir_dumpf("mod");
			break;

		case IR_IC_AND:
			ir_dumpf("and");
			break;
		case IR_IC_OR:
			ir_dumpf("or");
			break;
		case IR_IC_XOR:
			ir_dumpf("xor");
			break;


		case IR_IC_JMP:
			ir_dumpf("jmp");
			break;
		case IR_IC_JMPZ:
			ir_dumpf("jmpz");
			break;
		case IR_IC_JMPNZ:
			ir_dumpf("jmpnz");
			break;
		case IR_IC_JMPEQ:
			ir_dumpf("jmpeq");
			break;
		case IR_IC_JMPLT:
			ir_dumpf("jmplt");
			break;
		case IR_IC_JMPLE:
			ir_dumpf("jmple");
			break;

		case IR_IC_ITOF:
			ir_dumpf("itof");
			break;
		case IR_IC_FTOI:
			ir_dumpf("ftoi");
			break;

		case IR_IC_PARAM:
			ir_dumpf("param");
			break;
		case IR_IC_CALL:
			ir_dumpf("call");
			break;
		case IR_IC_RET:
			ir_dumpf("ret");
			break;

		case IR_IC_SLT:
			ir_dumpf("slt");
			break;

		default:
			unreachable();
			break;
	}
}

static void ir_dump_label_kind(const ir_builder *const builder, const ir_label_kind label_kind)
{
	(void) builder;

	switch (label_kind)
	{
		case IR_LABEL_KIND_END:
			ir_dumpf("END");
			break;
		case IR_LABEL_KIND_BEGIN:
			ir_dumpf("BEGIN");
			break;
		case IR_LABEL_KIND_THEN:
			ir_dumpf("THEN");
			break;
		case IR_LABEL_KIND_ELSE:
			ir_dumpf("ELSE");
			break;
		case IR_LABEL_KIND_BEGIN_CYCLE:
			ir_dumpf("BEGIN_CYCLE");
			break;
		case IR_LABEL_KIND_NEXT:
			ir_dumpf("NEXT");
			break;
		case IR_LABEL_KIND_AND:
			ir_dumpf("AND");
			break;
		case IR_LABEL_KIND_OR:
			ir_dumpf("OR");
			break;
		default:
			unreachable();
			break;
	}
}

static void ir_dump_label(const ir_builder *const builder, const ir_label *const label)
{
	const ir_label_kind kind = ir_label_get_kind(label);
	const item_t id = ir_label_get_id(label);

	ir_dump_label_kind(builder, kind);
	ir_dumpf("%" PRIitem, id);
}

static void ir_dump_n_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf("\n");
}
static void ir_dump_rn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf("\n");
}
static void ir_dump_rr_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf("\n");
}
static void ir_dump_rrn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t op2 = ir_instr_get_op2(instr);
	const ir_value op2_value = ir_get_value(builder, op2);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf(", ");
	ir_dump_value(builder, &op2_value);
	ir_dumpf("\n");
}
static void ir_dump_rrr_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op2(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t op2 = ir_instr_get_op2(instr);
	const ir_value op2_value = ir_get_value(builder, op2);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf(", ");
	ir_dump_value(builder, &op2_value);
	ir_dumpf("\n");
}
static void ir_dump_lr_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf("\n");
}
static void ir_dump_rl_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf("\n");
}
static void ir_dump_rln_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t op2 = ir_instr_get_op2(instr);
	const ir_value op2_value = ir_get_value(builder, op2);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_value(builder, &op1_value);
	ir_dumpf(", ");
	ir_dump_value(builder, &op2_value);
	ir_dumpf("\n");
}
static void ir_dump_sl_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_get_value(builder, op1);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dumpf("%" PRIitem, op1);
	ir_dumpf("\n");
}
static void ir_dump_bn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_label label_ = ir_get_label(builder, op1);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_label(builder, &label_);
	ir_dumpf("\n");
}
static void ir_dump_brn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_label label_ = ir_get_label(builder, op1);

	const item_t op2 = ir_instr_get_op2(instr);
	const ir_value op2_value = ir_get_value(builder, op2);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_label(builder, &label_);
	ir_dumpf(", ");
	ir_dump_value(builder, &op2_value);
	ir_dumpf("\n");
}
static void ir_dump_brrn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_label label_ = ir_get_label(builder, op1);

	const item_t op2 = ir_instr_get_op2(instr);
	const ir_value op2_value = ir_get_value(builder, op2);

	const item_t op3 = ir_instr_get_op3(instr);
	const ir_value op3_value = ir_get_value(builder, op3);


	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dump_label(builder, &label_);
	ir_dumpf(", ");
	ir_dump_value(builder, &op2_value);
	ir_dumpf(", ");
	ir_dump_value(builder, &op3_value);
	ir_dumpf("\n");
}
static void ir_dump_fn_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);

	ir_dumpf("\t");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dumpf("$%" PRIitem, op1);
	ir_dumpf("\n");
}
static void ir_dump_fr_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);

	const item_t op1 = ir_instr_get_op1(instr);

	const item_t res = ir_instr_get_res(instr);
	const ir_value res_value = ir_get_value(builder, res);

	ir_dumpf("\t");
	ir_dump_value(builder, &res_value);
	ir_dumpf(" <- ");
	ir_dump_ic(builder, ic);
	ir_dumpf(" ");
	ir_dumpf("$%" PRIitem, op1);
	ir_dumpf("\n");
}

static void ir_dump_instr(const ir_builder *const builder, const ir_instr *const instr)
{
	const ir_instr_kind instr_kind = ir_instr_get_kind(instr);

	switch (instr_kind)
	{
		case IR_INSTR_KIND_N:
			ir_dump_n_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RN:
			ir_dump_rn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RR:
			ir_dump_rr_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RRN:
			ir_dump_rrn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RRR:
			ir_dump_rrr_instr(builder, instr);
			break;
		case IR_INSTR_KIND_LR:
			ir_dump_lr_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RL:
			ir_dump_rl_instr(builder, instr);
			break;
		case IR_INSTR_KIND_RLN:
			ir_dump_rln_instr(builder, instr);
			break;
		case IR_INSTR_KIND_SL:
			ir_dump_sl_instr(builder, instr);
			break;
		case IR_INSTR_KIND_BN:
			ir_dump_bn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_BRN:
			ir_dump_brn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_BRRN:
			ir_dump_brn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_FN:
			ir_dump_fn_instr(builder, instr);
			break;
		case IR_INSTR_KIND_FR:
			ir_dump_fr_instr(builder, instr);
			break;
	}
}

static void ir_dump_extern(const ir_builder *const builder, const ir_extern *const extern_)
{
	const item_t id = ir_extern_get_id(extern_);
	const item_t type = ir_extern_get_type(extern_);

	ir_dumpf("extern ");
	ir_dump_type(builder, type);
	ir_dumpf("%" PRIitem "\n", id);
}
static void ir_dump_global(const ir_builder *const builder, const ir_global *const global)
{
	const item_t id = ir_global_get_id(global);
	const item_t type = ir_global_get_type(global);

	ir_dumpf("global ");
	ir_dump_type(builder, type);
	ir_dumpf("%" PRIitem "\n", id);
}
static void ir_dump_function(const ir_builder *const builder, const ir_function *const function)
{
	const item_t id = ir_function_get_id(function);
	const item_t type = ir_function_get_type(function);

	ir_dumpf("function ");
	ir_dump_type(builder, type);
	ir_dumpf(" ");
	ir_dumpf("$%" PRIitem "\n", id);

	ir_dumpf("{\n");
	for (size_t i = 0; i < ir_function_get_instr_count(function); i++)
	{
		const ir_instr instr = ir_function_index_instr(function, i);
		ir_dump_instr(builder, &instr);
	}
	ir_dumpf("}\n");
}


void ir_dump(const ir_builder *const builder)
{
	const ir_module *const module = builder->module;
	for (size_t i = 0; i < ir_module_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_module_index_extern(module, i);
		ir_dump_extern(builder, &extern_);
	}

	for (size_t i = 0; i < ir_module_get_global_count(module); i++)
	{
		ir_global global = ir_module_index_global(module, i);
		ir_dump_global(builder, &global);
	}

	for (size_t i = 0; i < ir_module_get_function_count(module); i++)
	{
		ir_function function = ir_module_index_function(module, i);
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
	const ir_module *const module = builder->module;
	const item_t id = expression_identifier_get_id(nd);

	return ir_idents_get(module, id);
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
			return ir_build_imm_int(builder, expression_literal_get_boolean(nd) ? 1 : 0);
		case TYPE_CHARACTER:
			return ir_build_imm_int(builder, expression_literal_get_character(nd));
		case TYPE_INTEGER:
			return ir_build_imm_int(builder, expression_literal_get_integer(nd));
		case TYPE_FLOATING:
			return ir_build_imm_float(builder, expression_literal_get_floating(nd));
		case TYPE_ARRAY:
			// TODO: Implement other array types.
			unimplemented();
			return IR_VALUE_VOID;
			//return ir_build_imm_string(builder, expression_literal_get_string(nd));
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
	unimplemented();
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
	//item_t imm_rvalue = ((operator == UN_PREINC) || (operator == UN_POSTINC)) ? builder->value_one : value_minus_one;

	if (is_prefix)
	{

	}
	else
	{

	}

	unimplemented();
	return IR_VALUE_VOID;
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

static item_t ir_emit_unary_expression(ir_builder *const builder, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);

	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			return ir_emit_increment_expression(builder, nd);

		case UN_MINUS:
		{
			const node operand = expression_unary_get_operand(nd);
			const item_t operand_value = ir_emit_expression(builder, &operand);
			const item_t minus_one_value = ir_build_imm_minus_one(builder);
			const item_t res_value = ir_emit_binary_operation(builder, operand_value, minus_one_value, BIN_MUL);
			ir_free_value(builder, minus_one_value);
			ir_free_value(builder, operand_value);
			return res_value;
		}
		case UN_NOT:
		{
			const node operand = expression_unary_get_operand(nd);
			const item_t operand_value = ir_emit_expression(builder, &operand);
			const item_t minus_one_value = ir_build_imm_minus_one(builder);
			const item_t res_value = ir_emit_binary_operation(builder, operand_value, minus_one_value, BIN_XOR);
			ir_free_value(builder, minus_one_value);
			ir_free_value(builder, operand_value);
			return res_value;
		}

		case UN_LOGNOT:
		{
			const node operand = expression_unary_get_operand(nd);
			const item_t value = ir_emit_expression(builder, &operand);
			//const item_t second_value = builder->value_zero;

			unimplemented();
			return IR_VALUE_VOID;
		}

		case UN_ABS:
		{
			unimplemented();
			return IR_VALUE_VOID;
		}

		case UN_ADDRESS:
		{
			const node operand = expression_unary_get_operand(nd);

			unimplemented();
			return IR_VALUE_VOID;
		}

		case UN_UPB:
		{
			unimplemented();
			return IR_VALUE_VOID;
		}

		default:
			unreachable();
			return IR_VALUE_VOID;
	}
}

static item_t ir_emit_binary_operation(ir_builder *const builder, const item_t lhs, const item_t rhs, const binary_t operator)
{
	assert(operator != BIN_LOG_AND);
	assert(operator != BIN_LOG_OR);

	const syntax *const sx = builder->sx;

	const ir_value lhs_value = ir_get_value(builder, lhs);
	const ir_value rhs_value = ir_get_value(builder, rhs);

	const item_t lhs_type = ir_value_get_type(&lhs_value);
	const item_t rhs_type = ir_value_get_type(&rhs_value);

	switch (operator)
	{
		case BIN_ADD:
		{
			if (type_is_integer(sx, lhs_type) && type_is_integer(sx, rhs_type))
			{
				//printf("%d %d\n", lhs, rhs);
				return ir_build_add(builder, lhs, rhs);
			}
			else 
			{
				const item_t lhs_value = type_is_floating(lhs_type) ? lhs : ir_build_itof(builder, lhs);
				const item_t rhs_value = type_is_floating(rhs_type) ? rhs : ir_build_itof(builder, rhs);
				return ir_build_fadd(builder, lhs_value, rhs_value);
			}
			break;
		}
		case BIN_SUB:
		{
			if (type_is_integer(sx, lhs_type) && type_is_integer(sx, rhs_type))
			{
				return ir_build_sub(builder, lhs, rhs);
			}
			else 
			{
				const item_t lhs_value = type_is_floating(lhs_type) ? lhs : ir_build_itof(builder, lhs);
				const item_t rhs_value = type_is_floating(rhs_type) ? rhs : ir_build_itof(builder, rhs);
				return ir_build_fsub(builder, lhs_value, rhs_value);
			}
			break;
		}
		case BIN_MUL:
		{
			if (type_is_integer(sx, lhs_type) && type_is_integer(sx, rhs_type))
			{
				return ir_build_mul(builder, lhs, rhs);
			}
			else 
			{
				const item_t lhs_value = type_is_floating(lhs_type) ? lhs : ir_build_itof(builder, lhs);
				const item_t rhs_value = type_is_floating(rhs_type) ? rhs : ir_build_itof(builder, rhs);
				return ir_build_fmul(builder, lhs_value, rhs_value);
			}
			break;
		}
		case BIN_DIV:
		{
			if (type_is_integer(sx, lhs_type) && type_is_integer(sx, rhs_type))
			{
				return ir_build_div(builder, lhs, rhs);
			}
			else 
			{
				const item_t lhs_value = type_is_floating(lhs_type) ? lhs : ir_build_itof(builder, lhs);
				const item_t rhs_value = type_is_floating(rhs_type) ? rhs : ir_build_itof(builder, rhs);

				return ir_build_fdiv(builder, lhs_value, rhs_value);
			}
		}
		case BIN_REM:
			return ir_build_mod(builder, lhs, rhs);
		case BIN_SHL:
			return ir_build_shl(builder, lhs, rhs);
		case BIN_SHR:
			return ir_build_shr(builder, lhs, rhs);

		case BIN_LT:
			unimplemented();
		case BIN_GT:
			unimplemented();
		case BIN_LE:
			unimplemented();
		case BIN_GE:
			unimplemented();
		case BIN_EQ:
			if (type_is_integer(sx, lhs_type) && type_is_integer(sx, rhs_type))
			{
				const item_t res = ir_build_temp(builder, TYPE_BOOLEAN);

				const item_t else_label = ir_add_label(builder, IR_LABEL_KIND_ELSE);
				const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

				ir_build_jmpeq(builder, else_label, lhs, rhs);

				const item_t zero_value = ir_build_imm_zero(builder);
				ir_build_move(builder, zero_value, res);
				ir_free_value(builder, zero_value);
				ir_build_jmp(builder, end_label);

				ir_build_label(builder, else_label);
				const item_t one_value = ir_build_imm_one(builder);
				ir_build_move(builder, one_value, res);
				ir_free_value(builder, one_value);

				ir_build_label(builder, end_label);

				ir_free_value(builder, lhs);
				ir_free_value(builder, rhs);

				return res;
			}
			else 
			{
				const item_t lhs_value = type_is_floating(lhs_type) ? lhs : ir_build_itof(builder, lhs);
				const item_t rhs_value = type_is_floating(rhs_type) ? rhs : ir_build_itof(builder, rhs);

				const item_t res = ir_build_temp(builder, TYPE_BOOLEAN);

				const item_t else_label = ir_add_label(builder, IR_LABEL_KIND_ELSE);
				const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

				ir_build_jmpeq(builder, else_label, lhs, rhs);

				//ir_build_move(builder, builder->value_zero, res);
				ir_build_jmp(builder, end_label);

				ir_build_label(builder, else_label);
				//ir_build_move(builder, builder->value_one, res);

				ir_build_label(builder, end_label);

				return res;
			}
			break;
		case BIN_NE:
		{
			unimplemented();
		}

		case BIN_AND:
			return ir_build_and(builder, lhs, rhs);
		case BIN_XOR:
			return ir_build_xor(builder, lhs, rhs);
		case BIN_OR:
			return ir_build_or(builder, lhs, rhs);

		default:
			unreachable();
			break;
	}
	return 0;
}

static item_t ir_emit_binary_expression(ir_builder *const builder, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const node lhs = expression_binary_get_LHS(nd);
	const node rhs = expression_binary_get_RHS(nd);

	switch (operator)
	{
		case BIN_COMMA:
		{
			ir_emit_void_expression(builder, &lhs);
			return ir_emit_expression(builder, &rhs);
		}
		case BIN_LOG_AND:
		{
			const item_t res_value = ir_build_temp(builder, TYPE_INTEGER);

			const item_t and_label = ir_add_label(builder, IR_LABEL_KIND_AND);
			const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

			const item_t lhs_value = ir_emit_expression(builder, &lhs);

			ir_build_jmpz(builder, and_label, lhs_value);
			ir_free_value(builder, lhs_value);

			const item_t rhs_value = ir_emit_expression(builder, &rhs);

			ir_build_jmpz(builder, and_label, rhs_value);
			ir_free_value(builder, rhs_value);

			const item_t one_value = ir_build_imm_one(builder);
			ir_build_move(builder, one_value, res_value);
			ir_free_value(builder, one_value);
			ir_build_jmp(builder, end_label);

			ir_build_label(builder, and_label);
			const item_t zero_value = ir_build_imm_zero(builder);
			ir_build_move(builder, zero_value, res_value);
			ir_free_value(builder, zero_value);

			ir_build_label(builder, end_label);

			return res_value;
		}
		case BIN_LOG_OR:
		{
			const item_t res_value = ir_build_temp(builder, TYPE_INTEGER);
			const item_t lhs_value = ir_emit_expression(builder, &lhs);

			const item_t or_label = ir_add_label(builder, IR_LABEL_KIND_OR);
			const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

			ir_build_jmpnz(builder, or_label, lhs_value);
			ir_free_value(builder, lhs_value);

			const item_t rhs_value = ir_emit_expression(builder, &rhs);

			ir_build_jmpnz(builder, or_label, rhs_value);
			ir_free_value(builder, rhs_value);

			const item_t zero_value = ir_build_imm_zero(builder);
			ir_build_move(builder, zero_value, res_value);
			ir_free_value(builder, zero_value);
			ir_build_jmp(builder, end_label);

			ir_build_label(builder, or_label);
			const item_t one_value = ir_build_imm_zero(builder);
			ir_build_move(builder, one_value, res_value);
			ir_free_value(builder, one_value);

			ir_build_label(builder, end_label);

			return res_value;
		}
		default:
		{
			const item_t lhs_value = ir_emit_expression(builder, &lhs);
			const item_t rhs_value = ir_emit_expression(builder, &rhs);
			const item_t res_value = ir_emit_binary_operation(builder, lhs_value, rhs_value, operator);
			ir_free_value(builder, lhs_value);
			ir_free_value(builder, rhs_value);
			return res_value;
		}
	}
}

static item_t ir_emit_ternary_expression(ir_builder *const builder, const node *const nd)
{
	const node condition = expression_ternary_get_condition(nd);
	const node lhs_node = expression_ternary_get_LHS(nd);
	const node rhs_node = expression_ternary_get_RHS(nd);

	const item_t condition_value = ir_emit_expression(builder, &condition);
	const item_t lhs_value = ir_emit_expression(builder, &lhs_node);
	const item_t rhs_value = ir_emit_expression(builder, &rhs_node); 

	const ir_value lhs_value_ = ir_get_value(builder, lhs_value);
	const item_t lhs_type = ir_value_get_type(&lhs_value_);

	// FIXME: support different types?.
	const item_t res_value = ir_build_temp(builder, lhs_type);

	const item_t second_label = ir_add_label(builder, IR_LABEL_KIND_ELSE);
	const item_t end_label = ir_add_label(builder, IR_LABEL_KIND_END);

	ir_build_jmpz(builder, second_label, condition_value);

	ir_build_move(builder, lhs_value, res_value);
	ir_build_jmp(builder, end_label);

	ir_build_label(builder, second_label);
	ir_build_move(builder, rhs_value, res_value);

	ir_build_label(builder, end_label);

	ir_free_value(builder, condition_value);
	ir_free_value(builder, lhs_value);
	ir_free_value(builder, rhs_value);

	return res_value;
}

static item_t ir_emit_struct_assignment(ir_builder *const builder, const item_t target, const node *const value)
{
	unimplemented();
	return IR_VALUE_VOID;
}

static binary_t ir_get_convient_operator(const binary_t operator)
{
	binary_t correct_operation;
	switch (operator)
	{
		case BIN_ADD_ASSIGN:
			return BIN_ADD;
		case BIN_SUB_ASSIGN:
			return BIN_SUB;
		case BIN_MUL_ASSIGN:
			return BIN_MUL;
		case BIN_DIV_ASSIGN:
			return BIN_DIV;
		case BIN_SHL_ASSIGN:
			return BIN_SHL;
		case BIN_SHR_ASSIGN:
			return BIN_SHR;
		case BIN_AND_ASSIGN:
			return BIN_AND;
		case BIN_XOR_ASSIGN:
			return BIN_XOR;
		case BIN_OR_ASSIGN:
			return BIN_OR;
		default:
			return operator;
	}
}
static item_t ir_emit_assignment_expression(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const node lhs = expression_assignment_get_LHS(nd);
	const item_t target = ir_emit_lvalue(builder, &lhs);

	const node rhs = expression_assignment_get_RHS(nd);
	const item_t rhs_type = expression_get_type(&rhs);

	const binary_t operator = expression_assignment_get_operator(nd);
	const binary_t convient_operator = ir_get_convient_operator(operator);

	const item_t value = ir_emit_expression(builder, &rhs);
	ir_build_store(builder, value, target);

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
	unimplemented();
}

static void ir_emit_variable_declaration(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;
	ir_module *module = builder->module;

	const size_t identifier = declaration_variable_get_id(nd);
	const item_t type = ident_get_type(sx, identifier);

	const item_t var_ptr_value = ir_build_alloca(builder, type);

	ir_idents_add(
		module,
		identifier,
		var_ptr_value
	);

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

		ir_build_store(builder, value, var_ptr_value);
		ir_free_value(builder, value);
		// }
	}
}

static void ir_emit_function_definition(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;
	const item_t id = (item_t) declaration_function_get_id(nd);


	const item_t func_type = ident_get_type(sx, id);
	//const size_t parameter_count = type_function_get_parameter_amount(sx, func_type);

	ir_build_function_definition(builder, id, func_type);

	node body = declaration_function_get_body(nd);

	ir_displ_reset(builder);
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
// Генерация IR из выражений.
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
	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		ir_emit_statement(builder, &substmt);
	}
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
		ir_build_jmpnz(builder, end_label, value);
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
}

static void ir_emit_return_statement(ir_builder *const builder, const node *const nd)
{
	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		const item_t value = ir_emit_expression(builder, &expression);

		ir_build_ret(builder, value);
		ir_free_value(builder, value);
	}
	else 
	{
		ir_build_ret(builder, IR_VALUE_VOID);
	}
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

void ir_emit_module(ir_builder *const builder, const node *const nd)
{
	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = translation_unit_get_declaration(nd, i);
		ir_emit_declaration(builder, &decl);
	}

	// FIXME: Обработка ошибок.
}

//
// Обход IR дерева.
//

typedef struct ir_context
{
	const ir_module *const module;
	const ir_gens *const gens;
	void *const enc;
} ir_context;

ir_context create_ir_context(const ir_module *const module, const ir_gens *const gens, void *const enc)
{
	return (ir_context) {
		.module = module,
		.gens = gens,
		.enc = enc
	};
}

static rvalue ir_value_to_rvalue(const ir_value *const value)
{
	switch (ir_value_get_kind(value))
	{
		case IR_VALUE_KIND_IMM:
			return (rvalue) {

			};
		case IR_VALUE_KIND_TEMP:
			return create_temp_rvalue(ir_temp_value_get_id(value));
			return (rvalue) {
				.kind = RVALUE_KIND_TEMP,
				.id = ir_temp_value_get_id(value)
			};
		default:
			return (rvalue) {

			};
	}

}
static lvalue ir_value_to_lvalue(const ir_value *const value)
{
	switch (ir_value_get_kind(value))
	{
		case IR_VALUE_KIND_LOCAL:
		case IR_VALUE_KIND_PARAM:
		case IR_VALUE_KIND_GLOBAL:
			break;
		default:
			unreachable();
			break;
	}

	return (lvalue) {};
}
static label ir_to_label(const ir_label *const label_)
{
	return (label) {};
}

static ir_gen_n_instr_func ir_get_n_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_NOP:
			return gens->gen_nop;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rn_instr_func ir_get_rn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_PARAM:
			return gens->gen_param;
		case IR_IC_RET:
			return gens->gen_ret;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rr_instr_func ir_get_rr_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_FTOI:
			return gens->gen_ftoi;
		case IR_IC_ITOF:
			return gens->gen_itof;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rrn_instr_func ir_get_rrn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_MOVE:
			return gens->gen_move;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rrr_instr_func ir_get_rrr_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_ADD:
			return gens->gen_add;
		case IR_IC_SUB:
			return gens->gen_sub;
		case IR_IC_MUL:
			return gens->gen_mul;
		case IR_IC_DIV:
			return gens->gen_div;
		case IR_IC_MOD:
			return gens->gen_mod;
		case IR_IC_AND:
			return gens->gen_and;
		case IR_IC_OR:
			return gens->gen_or;
		case IR_IC_XOR:
			return gens->gen_xor;
		case IR_IC_FADD:
			return gens->gen_fadd;
		case IR_IC_FSUB:
			return gens->gen_fsub;
		case IR_IC_FMUL:
			return gens->gen_fmul;
		case IR_IC_FDIV:
			return gens->gen_fdiv;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_lr_instr_func ir_get_lr_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_LOAD:
			return gens->gen_load;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rl_instr_func ir_get_rl_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_rln_instr_func ir_get_rln_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_STORE:
			return gens->gen_store;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_sl_instr_func ir_get_sl_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_ALLOCA:
			return gens->gen_alloca;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_bn_instr_func ir_get_bn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_LABEL:
			return gens->gen_label;
		case IR_IC_JMP:
			return gens->gen_jmp;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_brn_instr_func ir_get_brn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_JMPZ:
			return gens->gen_jmpz;
		case IR_IC_JMPNZ:
			return gens->gen_jmpnz;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_brrn_instr_func ir_get_brrn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_JMPEQ:
			return gens->gen_jmpeq;
		case IR_IC_JMPLT:
			return gens->gen_jmplt;
		case IR_IC_JMPLE:
			return gens->gen_jmple;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_fn_instr_func ir_get_fn_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_fr_instr_func ir_get_fr_instr_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_ic ic = ir_instr_get_ic(instr);
	const ir_gens *const gens = ctx->gens;

	switch (ic)
	{
		case IR_IC_CALL:
			return gens->gen_call;
		default:
			unreachable();
			return NULL;
	}
}
static ir_gen_extern_func ir_get_extern_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_extern;
}
static ir_gen_global_func ir_get_global_gen(const ir_context *const ctx, const ir_instr *const instr)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_global;
}

static ir_gen_function_func ir_get_function_begin_gen(const ir_context *const ctx)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_function_begin;
}
static ir_gen_function_func ir_get_function_end_gen(const ir_context *const ctx)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_function_end;
}
static ir_gen_general_func ir_get_begin_gen(const ir_context *const ctx)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_begin;
}
static ir_gen_general_func ir_get_end_gen(const ir_context *const ctx)
{
	const ir_gens *const gens = ctx->gens;
	return gens->gen_end;
}


static void ir_gen_n_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;
}
static void ir_gen_rn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const ir_value op1_value = ir_module_get_value(module, op1);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);

	ir_get_rn_instr_gen(ctx, instr)(enc, &op1_rvalue);
}
static void ir_gen_rr_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value res_value = ir_module_get_value(module, res);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);
	const rvalue res_rvalue = ir_value_to_rvalue(&res_value);

	ir_get_rr_instr_gen(ctx, instr)(enc, &op1_rvalue, &res_rvalue);
}
static void ir_gen_rrn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t op2 = ir_instr_get_op2(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value op2_value = ir_module_get_value(module, op2);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);
	const rvalue op2_rvalue = ir_value_to_rvalue(&op2_value);

	ir_get_rrn_instr_gen(ctx, instr)(enc, &op1_rvalue, &op2_rvalue);
}
static void ir_gen_rrr_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t op2 = ir_instr_get_op2(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value op2_value = ir_module_get_value(module, op2);
	const ir_value res_value = ir_module_get_value(module, res);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);
	const rvalue op2_rvalue = ir_value_to_rvalue(&op2_value);
	const rvalue res_rvalue = ir_value_to_rvalue(&res_value);

	ir_get_rrr_instr_gen(ctx, instr)(enc, &op1_rvalue, &op2_rvalue, &res_rvalue);
}
static void ir_gen_lr_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value res_value = ir_module_get_value(module, res);

	const lvalue op1_lvalue = ir_value_to_lvalue(&op1_value);
	const rvalue res_rvalue = ir_value_to_rvalue(&res_value);

	ir_get_lr_instr_gen(ctx, instr)(enc, &op1_lvalue, &res_rvalue);
}
static void ir_gen_rl_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value res_value = ir_module_get_value(module, res);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);
	const lvalue res_lvalue = ir_value_to_lvalue(&res_value);

	ir_get_rl_instr_gen(ctx, instr)(enc, &op1_rvalue, &res_lvalue);
}
static void ir_gen_rln_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t op2 = ir_instr_get_op2(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value op2_value = ir_module_get_value(module, op2);

	const rvalue op1_rvalue = ir_value_to_rvalue(&op1_value);
	const lvalue op2_lvalue = ir_value_to_lvalue(&op2_value);

	ir_get_rln_instr_gen(ctx, instr)(enc, &op1_rvalue, &op2_lvalue);
}
static void ir_gen_sl_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value op1_value = ir_module_get_value(module, op1);
	const ir_value res_value = ir_module_get_value(module, res);

	const size_t size = (size_t) op1;
	const lvalue res_lvalue = ir_value_to_lvalue(&res_value);

	ir_get_sl_instr_gen(ctx, instr)(enc, size, &res_lvalue);
}
static void ir_gen_bn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);

	const ir_value op1_value = ir_module_get_label(module, op1);

	const label op1_label = ir_to_label(&op1_value);

	ir_get_bn_instr_gen(ctx, instr)(enc, &op1_label);
}
static void ir_gen_brn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t op2 = ir_instr_get_op2(instr);

	const ir_label op1_value = ir_module_get_label(module, op1);
	const ir_value op2_value = ir_module_get_value(module, op2);

	const label op1_label = ir_to_label(&op1_value);
	const rvalue op2_rvalue = ir_value_to_rvalue(&op2_value);

	ir_get_brn_instr_gen(ctx, instr)(enc, &op1_label, &op2_rvalue);
}
static void ir_gen_brrn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t op2 = ir_instr_get_op2(instr);
	const item_t op3 = ir_instr_get_op3(instr);

	const ir_label op1_value = ir_module_get_label(module, op1);
	const ir_value op2_value = ir_module_get_value(module, op2);
	const ir_value op3_value = ir_module_get_value(module, op3);

	const label op1_label = ir_to_label(&op1_value);
	const rvalue op2_rvalue = ir_value_to_rvalue(&op2_value);
	const rvalue op3_rvalue = ir_value_to_rvalue(&op3_value);

	ir_get_brrn_instr_gen(ctx, instr)(enc, &op1_label, &op2_rvalue, &op3_rvalue);
}
static void ir_gen_fn_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);

	const item_t op1_function = op1;

	ir_get_fn_instr_gen(ctx, instr)(enc, op1_function);
}
static void ir_gen_fr_instr(ir_context *const ctx, const ir_instr *const instr)
{
	const ir_module *const module = ctx->module;
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t op1 = ir_instr_get_op1(instr);
	const item_t res = ir_instr_get_res(instr);

	const ir_value res_value = ir_module_get_label(module, res);

	const item_t op1_function = op1;
	const rvalue res_rvalue = ir_value_to_rvalue(&res_value);

	ir_get_fr_instr_gen(ctx, instr)(enc, op1_function, &res_rvalue);
}

static void ir_gen_instr(ir_context *const ctx, const ir_instr *const instr)
{
	// На данный момент все инструкции имеют вид %r = ic %1 %2
	// Однако, возможно, в будущем будут использоваться функции с бОльшим числом
	// операндов, поэтому резонно передавать в ir_gen_* функции instr.
	const ir_instr_kind ic_kind = ir_instr_get_kind(instr);

	switch (ic_kind)
	{
		case IR_INSTR_KIND_N:
			ir_gen_n_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RN:
			ir_gen_rn_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RR:
			ir_gen_rr_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RRN:
			ir_gen_rrn_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RRR:
			ir_gen_rrr_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_LR:
			ir_gen_lr_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RL:
			ir_gen_rl_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_RLN:
			ir_gen_rln_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_SL:
			ir_gen_sl_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_BN:
			ir_gen_bn_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_BRN:
			ir_gen_brn_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_FN:
			ir_gen_fn_instr(ctx, instr);
			break;
		case IR_INSTR_KIND_FR:
			ir_gen_fr_instr(ctx, instr);
			break;
		default:
			unreachable();
			break;
	}
}
static void ir_gen_extern(ir_context *const ctx, const ir_extern *const extern_)
{
	const ir_gens *const gens = ctx->gens;
	const void *const enc = ctx->enc;

	const item_t id = ir_extern_get_id(extern_);
	const item_t type = ir_extern_get_type(extern_);


}
static void ir_gen_global(ir_context *const ctx, const ir_global *const global)
{
	
}
static void ir_gen_function(ir_context *const ctx, const ir_function *const function)
{
	const ir_gens *const gens = ctx->gens;
	void *const enc = ctx->enc;

	const item_t id = ir_function_get_id(function);
	const item_t type = ir_function_get_type(function);

	ir_get_function_begin_gen(ctx)(enc, id, type);
	for (size_t i = 0; i < ir_function_get_instr_count(function); i++)
	{
		ir_instr instr = ir_function_index_instr(function, i);
		ir_gen_instr(ctx, &instr);
	}
	ir_get_function_end_gen(ctx)(enc, id, type);
}
void ir_gen_module(const ir_module *const module, const ir_gens *const gens, void *const enc)
{
	ir_context ctx = create_ir_context(module, gens, enc);

	ir_get_begin_gen(&ctx)(enc);
	for (size_t i = 0; i < ir_module_get_extern_count(module); i++)
	{
		ir_extern extern_ = ir_module_index_extern(module, i);
		ir_gen_extern(&ctx, &extern_);
	}
	for (size_t i = 0; i < ir_module_get_global_count(module); i++)
	{
		ir_global global = ir_module_index_global(module, i);
		ir_gen_global(&ctx, &global);
	}
	for (size_t i = 0; i < ir_module_get_function_count(module); i++)
	{
		ir_function function = ir_module_index_function(module, i);
		ir_gen_function(&ctx, &function);
	}
	ir_get_end_gen(&ctx)(enc);
}

