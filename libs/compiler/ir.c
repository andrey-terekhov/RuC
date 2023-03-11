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


//
// Утилиты.
//


#ifdef DEBUG

#define unimplemented(msg)\
	do {\
		printf("Используется нереализованная фича: %s (смотри %s:%d)\n", msg, __FILE__, __LINE__);\
		system_error(node_unexpected);\
	} while(0)

#else

inline unimplemented(const char* msg)
{
	printf("Код для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}

#endif

//
// Значения.
//

#define IR_MAX_TEMP_VALUES 8

typedef enum ir_value_kind
	IR_VALUE_KIND_VOID,
	IR_VALUE_KIND_CONST,
	IR_VALUE_KIND_TEMP,
	IR_VALUE_KIND_LOCAL,
	IR_VALUE_KIND_GLOBAL,
	IR_VALUE_KIND_PARAM
} ir_value_kind;

typedef node ir_value;

static ir_value create_ir_value(const node *const nd, ir_value_kind kind)
{
	return node_add_child(nd, kind);
}

static ir_value create_ir_const_value(const node *const nd)
{
	return create_ir_value(nd, IR_VALUE_KIND_CONST);
}
static ir_value create_ir_const_int(const node *const nd, int value)
{
	ir_value value = create_ir_const_value(nd);
	node_add_arg(&value, value);
	return value;
}
static ir_value create_ir_const_float(const node *const nd, double value)
{
	ir_value value = create_ir_const_value();
	node_add_arg(&value, value);
	return value;
}
static ir_value create_ir_const_string(const node *const nd, char* value)
{
	unimplemented();
	return node_broken();
}

static ir_value create_ir_temp_value(const node *const nd, size_t id)
{
	ir_value value = create_ir_value(nd, IR_VALUE_KIND_TEMP);
	node_add_arg(&value, id);
	return value;
}

static ir_value_kind ir_value_get_kind(const ir_value *const value)
{
	return node_get_type(value);
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

// ic <=> instruction code.
typedef enum ir_ic
{
	IR_IC_NOP,

	IR_IC_LABEL,

	IR_IC_STORE,
	IR_IC_LOAD,
	IR_IC_ALLOCA,

	IR_IC_PTR,
	IR_IC_MOV,

	IR_IC_ADD,
	IR_IC_SUB,
	IR_IC_MUL,
	IR_IC_DIV,

	IR_IC_FADD,
	IR_IC_FSUB,
	IR_IC_FMUL,
	IR_IC_FDIV,

	IR_IC_MOD,
	IR_IC_INC,
	IR_IC_DEC,

	IR_IC_JMP,
	IR_IC_JMPC,

	IR_IC_SELECT
} ir_ic;

typedef node ir_instr;

static ir_instr create_ir_instr(const node *const nd, const ir_ic ic, const item_t op1, const item_t op2, const item_t res)
{
	ir_instr instr = node_add_child(nd, ic);
	node_add_arg(instr, 0, op1);
	node_add_arg(instr, 1, op2);
	node_add_arg(instr, 2, res);
	return instr;
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

// На данный момент метки реализованы при помощи инструкций типа меток, где
// op1 - тип метки, op2 - их айди.
//
// Метод костыльный, но проще в реализации.

typedef enum ir_label_kind
{
	IR_LABEL_BEGIN,
	IR_LABEL_THEN,
	IR_LABEL_ELSE,
	IR_LABEL_END
} ir_label_kind;

typedef node ir_label;

static ir_label create_ir_label(const node *const nd, ir_label_kind kind)
{
	static int id = 0;

	ir_label label = node_add_child(nd, kind);
	node_add_arg(label, ++id);
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

static void create_ir_function(const node *const nd, item_t id, item_t type)
{
	ir_function function = node_add_child(nd);
	node_add_arg(function id);
	node_add_arg(function, type);
	return function;
}

static void ir_function_get_instr_count(const ir_function *const function)
{
	return node_get_amount(function);
}
static ir_instr ir_function_get_instr(const ir_function *const function, size_t number)
{
	return node_get_child(function, number);
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

static size_t ir_module_get_extern_count(ir_module *const module)
{
	return node_get_amount(&module->externs_root);
}
static ir_extern ir_module_get_extern(ir_module *const module, size_t index)
{
	return node_get_child(&module->externs_root, index);
}

static size_t ir_module_get_global_count(ir_module *const module)
{
	return node_get_amount(&module->globals_root);
}
static ir_global ir_module_get_global(ir_module *const module, size_t index)
{
	return node_get_child(&module->globals_root, index);
}

static size_t ir_module_get_function_count(ir_module *const module)
{
	return node_get_amount(&module->functions_root);
}
static ir_function ir_module_get_function(ir_module *const module, size_t index)
{
	return node_get_child(&module->functions_root, index);
}

static size_t ir_module_get_value_count(ir_module *const module)
{
	return node_get_amount(&module->values_root);
}
static ir_value ir_module_get_value(ir_module *const module, size_t index)
{
	return node_get_child(&module->values_root, index);
}

static size_t ir_module_get_label_count(ir_module *const module)
{
	return node_get_amount(&module->labels_root);
}
static ir_label ir_module_get_label(ir_module *const module, size_t index)
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

	ir_value temp_values[IR_MAX_TEMP_VALUES];
	bool temp_used[IR_MAX_TEMP_VALUES];

	item_t break_label;
	item_t continue_label;
	item_t function_end_label;
}


static ir_builder create_ir_builder(ir_module* module, syntax *const sx) 
{
	return (ir_builder) {
		.sx = sx;
		.module = module;
	};
}

// Функции создания инструкций.

static void ir_build_extern(ir_builder *const builder, item_t id, item_t type)
{
	create_ir_extern(&builder->module->externs_root, id, type);
}

static void ir_build_global(ir_builder *const builder, item_t id, item_t type)
{
	create_ir_global(&builder->module->globals_root, id, type)
}

// Возможно, размеры типов будут платформозависимы, но в общем случае лучше уже на этапе генерации IR
// знать размеры типов.

const size_t IR_INT_TYPE_SIZE = 4;
const size_t IR_FLOAT_TYPE_SIZE = 4;
const size_t IR_BOOL_TYPE_SIZE = 1;
const size_t IR_CHAR_TYPE_SIZE = 4;

const size_t IR_POINTER_TYPE_SIZE = 4;

static size_t ir_get_type_size(ir_builder *const builder, item_t type)
{
	const syntax *const sx = builder->sx;
	return type_size(sx, type);
}


// Building values.

static item_t ir_build_temp(ir_builder *const builder)
{
	ir_value value = create_ir_temp_value(&builder->module->values_root);
	return ir_value_save(value);
}
static item_t ir_build_const_int(ir_builder *const builder, int value)
{
	ir_value value = create_ir_const_int(&builder->module->values_root, value);
	return ir_value_save(value);
}
static item_t ir_build_const_float(ir_builder *const builder, float value)
{
	ir_value value = create_ir_const_float(&builder->module->values_root, value);
	return ir_value_save(value);
}
static item_t ir_build_const_string(ir_builder *const builder, char* value)
{
	size_t size = strlen(value);
	// Stub.
	unimplemented();
	return -1;
}

static ir_value ir_get_value(ir_builder *const builder, item_t id)
{
	return node_load(&builder->module->values, (size_t) id);
}

static item_t ir_free_value(ir_builder *const builder, item_t value)
{
	const ir_value = node_load(builder->values, value);
	if (!ir_value_is_temp(ir_value))

}

static item_t(ir_builder *const builder)

static void ir_build_label(ir_builder *const builder, ir_label* label)
{
	ir_function_add_instr(builder->function, label);
}

static void ir_build_instr(ir_builder* const builder, ir_ic ic, ir_value* op1, ir_value* op2, ir_value* res)
{
	ir_instr instr = create_ir_instr(ic, op1, op2, res);
	node_vector_add(builder->instrs, instr);
}

// etc.
static void ir_build_nop(ir_builder* builder)
{
	ir_build_instr(builder, IR_IC_NOP, NULL, NULL, NULL);
}

static item_t ir_build_alloca(ir_builder* builder, item_t type)
{
	ir_build_instr(builder, IR_IC_ALLOCA, ir_get_type_size(builder, type), IR_VALUE_NULL);
}

static ir_value* ir_build_temp_value(ir_builder *const builder)
{
	size_t temp_id = 0;
	while(builder->temp_used[temp_id])
		temp_id++;

	builder->temp_values[temp_id] = (ir_value) {

	};
	return &builder->temp[temp_id];
}

static ir_value* ir_build_cti()
{

}
static ir_value* ir_build_ctf()
{

}

static item_t ir_build_bin(ir_builder* builder, ir_ic ic, item_t lhs, item_t rhs)
{
	const ir_value lhs_value = ir_get_value(builder, lhs);
	const ir_value rhs_value = ir_get_value(builder, rhs);

	if (ir_value_is_const(lhs) && ir_value_is_const(rhs))
	{
		int lhs_value = ir_value_get_const_int(&lhs_value);
		int rhs_value = ir_value_get_const_int(&rhs_value);
		int res_value = 0;

		switch (ir_ic)
		{
		case IR_IC_ADD:
			res_value = lhs_value + rhs_value;
			break;
		case IR_IC_SUB:
			res_value = lhs_value - rhs_value;
			break;
		case IR_IC_MUL:
			res_value = lhs_value * rhs_value;
			break;
		case IR_IC_DIV:
			res_value = lhs_value / rhs_value;
			break;
		}

		return ir_build_const_int(builder, res_value);
	}

	item_t res = create_ir_temp_value(builder->type_int);
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

static item_t ir_build_fbin(const ir_builder *const builder, const ir_ic ic, const item_t lhs, const item_t rhs)
{
	if (lhs->kind == IR_VALUE_KIND_CONST && rhs->kind == IR_VALUE_KIND_CONST)
	{
		float lhs_value = ir_builder_get_value(builder, lhs);
		float rhs_value = ir_builder_get_value(builder, rhs);
		float res_value = 0;

		switch (ir_ic)
		{
		case IR_IC_ADD:
			res_value = lhs_value + rhs_value;
			break;
		case IR_IC_SUB:
			res_value = lhs_value - rhs_value;
			break;
		case IR_IC_MUL:
			res_value = lhs_value * rhs_value;
			break;
		case IR_IC_DIV:
			res_value = lhs_value / rhs_value;
			break;
		}

		return create_ir_const_int(res_value);
	}

	ir_value* res = create_ir_temp_value(builder->type_int);
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


static void ir_build_jump(ir_builder* builder, ir_label* label)
{
	ir_build_instr(builder, IR_IC_JMP, label, NULL, NULL);
}
static void ir_build_jump_cond(ir_builder* builder, ir_label* label, ir_value* cond)
{
	ir_build_instr(builder, IR_IC_JMPC, label, NULL, NULL);
}



//
// Генерация выражений.
//

static item_t ir_emit_expression(ir_builder *const builder, const node *const nd);

static item_t ir_emit_identifier_lvalue(ir_builder* const builder, const node *const nd)
{
	return displacements_get(builder, expression_identifier_get_id(nd));
}

static item_t ir_emit_subscript_lvalue(ir_builder* const builder, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const node base = expression_subscript_get_base(nd);
	const node index = expression_subscript_get_index(nd);

	const item_t base_value = ir_emit_expression(builder, &base);
	const item_t index_value = ir_emit_expression(builder, &index);

	const item_t res_value = ir_build_ptr(builder, base_value, index_value);

	ir_free_value(base_value);
	ir_free_value(index_value);

	return res_value;
}

static item_t ir_emit_member_lvalue(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;
	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);

	const bool is_arrow = expression_member_is_arrow(nd);
	const item_t struct_type = is_arrow ? type_pointer_get_element_type(enc->sx, base_type) : base_type;

	size_t member_displ = 0;
	const size_t member_index = expression_member_get_member_index(nd);
	for (size_t i = 0; i < member_index; i++)
	{
		const item_t member_type = type_structure_get_member_type(sx, struct_type, i);
		member_displ += type_size(member_type) * 4;
	}

	const item_t type = expression_get_type(nd);

	if (is_arrow)
	{
		const item_t struct_pointer = ir_emit_expression(builder, &base);
		// FIXME: грузить константу на регистр в случае константных указателей
		return ir_build_ptr(builder, struct_pointer, member_displ);
		return (lvalue) {
			.kind = LVALUE_KIND_STACK,
			.base_reg = struct_pointer.val.reg_num,
			.loc.displ = member_displ,
			.type = type
		};
	}
	else
	{
		const lvalue base_lvalue = emit_lvalue(enc, &base);
		const size_t displ = (size_t)(base_lvalue.loc.displ + member_displ);
		return (lvalue) {
			.kind = LVALUE_KIND_STACK,
			.base_reg = base_lvalue.base_reg,
			.loc.displ = displ,
			.type = type
		};
	}
}

static item_t ir_emit_indirection(ir_builder *const builder, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);
	const item_t type = expression_get_type(nd);

	const item_t base_value = ir_emit_expression(builder, &operand);
	// FIXME: грузить константу на регистр в случае константных указателей
	const item_t res_value = ir_build_ptr(builder, base_value);

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

static void ir_emit_move_rvalue_to_register(encoder *const enc
	, const mips_register_t target, const rvalue *const value)
{
	if (value->kind == RVALUE_KIND_CONST)
	{
		const mips_instruction_t instruction = !type_is_floating(value->type) ? IC_MIPS_LI : IC_MIPS_LI_S;
		uni_printf(enc->sx->io, "\t");
		instruction_to_io(enc->sx->io, instruction);
		uni_printf(enc->sx->io, " ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ", ");
		rvalue_to_io(enc, value);
		uni_printf(enc->sx->io, "\n");
		return;
	}

	if (value->val.reg_num == target)
	{
		uni_printf(enc->sx->io, "\t# stays in register ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ":\n");
	}
	else
	{
		const mips_instruction_t instruction = !type_is_floating(value->type) ? IC_MIPS_MOVE : IC_MIPS_MFC_1;
		uni_printf(enc->sx->io, "\t");
		instruction_to_io(enc->sx->io, instruction);
		uni_printf(enc->sx->io, " ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ", ");
		rvalue_to_io(enc, value);
		uni_printf(enc->sx->io, "\n");
	}
}

static void ir_emit_store(ir_builder* *const enc, const ir_value *const src, const ir_value *dest)
{
	assert(value->kind != RVALUE_KIND_VOID);
	assert(value->type == target->type);

	const rvalue reg_value = (value->kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, value) : *value;

	if (target->kind == LVALUE_KIND_REGISTER)
	{
		if (value->val.reg_num != target->loc.reg_num)
		{
			const mips_instruction_t instruction = type_is_floating(value->type) ? IC_MIPS_MOV_S : IC_MIPS_MOVE;
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, instruction);
			uni_printf(enc->sx->io, " ");
			lvalue_to_io(enc, target);
			uni_printf(enc->sx->io, ", ");
			rvalue_to_io(enc, &reg_value);
			uni_printf(enc->sx->io, "\n");
		}
	}
	else
	{
		if ((!type_is_structure(enc->sx, target->type)) && (!type_is_array(enc->sx, target->type)))
		{
			const mips_instruction_t instruction = type_is_floating(value->type) ? IC_MIPS_S_S : IC_MIPS_SW;
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, instruction);
			uni_printf(enc->sx->io, " ");
			rvalue_to_io(enc, &reg_value);
			uni_printf(enc->sx->io, ", ");
			lvalue_to_io(enc, target);
			uni_printf(enc->sx->io, "\n");

			// Освобождаем регистр только в том случае, если он был занят на этом уровне. Выше не лезем.
			if (value->kind == RVALUE_KIND_CONST)
			{
				free_rvalue(enc, &reg_value);
			}

			free_register(enc, target->base_reg);
		}
		else
		{
			if (type_is_array(enc->sx, target->type))
			{
				// Загружаем указатель на массив
				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_SW);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, &reg_value);
				uni_printf(enc->sx->io, ", %" PRIitem "(", target->loc.displ);
				mips_register_to_io(enc->sx->io, target->base_reg);
				uni_printf(enc->sx->io, ")\n\n");
				return;
			}
			// else кусок должен быть не достижим
		}
	}
}


static ir_value* ir_emit_literal_expression(ir_builder* const builder, const node *const nd)
{
	item_t type = expression_get_type(nd);
	switch (type_get_class(context->sx, type))
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

static ir_value ir_emit_call_expression(ir_builder *const builder, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	// Конвертируем в указатель на функцию
	// FIXME: хотим рассмотреть любой callee как указатель
	// на данный момент это не поддержано в билдере, когда будет сделано -- добавить в emit_expression()
	// и применяем функцию emit_identifier_expression (т.к. его категория в билдере будет проставлена как rvalue)

	const size_t func_ref = expression_identifier_get_id(&callee);
	const size_t params_amount = expression_call_get_arguments_amount(nd);
	const item_t return_type = type_function_get_return_type(enc->sx, expression_get_type(&callee));

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
		ir_build_param(builder, tmp);
		ir_free_rvalue(enc, arg_rvalue);
	}

		const label label_func = { .kind = L_FUNC, .num = func_ref };
		emit_unconditional_branch(enc, IC_MIPS_JAL, &label_func);

		// Восстановление регистров-аргументов -- они могут понадобится в дальнейшем
		uni_printf(enc->sx->io, "\n\t# data restoring:\n");

		size_t i = 0, j = 0;  // Счётчик обычных и floating point регистров-аргументов соответственно
		while (i + j < arg_reg_count)
		{
			uni_printf(enc->sx->io, "\n");

			const rvalue tmp_rval = emit_load_of_lvalue(enc, &prev_arg_displ[i + j]);
			emit_move_rvalue_to_register(
				enc,
				type_is_floating(prev_arg_displ[i + j].type) ? (R_FA0 + 2 * j++) : (R_A0 + i++),
				&tmp_rval
			);

			free_rvalue(enc, &tmp_rval);
		}

		if (displ_for_parameters)
		{
			to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, (item_t)displ_for_parameters);
		}

		uni_printf(enc->sx->io, "\n");

	return (rvalue) {
		.kind = RVALUE_KIND_REGISTER,
		.type = return_type,
		.val.reg_num = type_is_floating(return_type) ? R_FV0 : R_V0,
		.from_lvalue = !FROM_LVALUE
	};
}

static ir_value* ir_emit_member_expression(encoder *const enc, const node *const nd)
{
	(void)enc;
	(void)nd;
	// FIXME: возврат структуры из функции. Указателя тут оказаться не может
	return RVALUE_VOID;
}

static ir_value* ir_emit_cast_expression(encoder *const enc, const node *const nd)
{
	const node operand = expression_cast_get_operand(nd);
	const rvalue value = emit_expression(enc, &operand);

	const item_t target_type = expression_get_type(nd);
	const item_t source_type = expression_get_type(&operand);

	if (type_is_integer(enc->sx, source_type) && type_is_floating(target_type))
	{
		// int -> float
		const rvalue result = {
			.kind = RVALUE_KIND_REGISTER,
			.from_lvalue = !FROM_LVALUE,
			.val.reg_num = get_float_register(enc),
			.type = target_type
		};

		// FIXME: избавится от to_code функций
		to_code_2R(enc->sx->io, IC_MIPS_MFC_1, value.val.reg_num, result.val.reg_num);
		to_code_2R(enc->sx->io, IC_MIPS_CVT_S_W, result.val.reg_num, result.val.reg_num);

		free_rvalue(enc, &value);
		return result;
	}
	else
	{
		// char -> int пока не поддержано в билдере
		return (rvalue) {
			.from_lvalue = value.from_lvalue,
			.kind = value.kind,
			.val.int_val = value.val.int_val,
			.type = TYPE_INTEGER
		};
	}
}

static item_t ir_emit_increment_expression(ir_builder *const builder, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);
	const unary_t operator = expression_unary_get_operator(nd);
	const bool is_prefix = (operator == UN_PREDEC) || (operator == UN_PREINC);

	item_t operand_value = ir_emit_lvalue(builder, &operand);
	item_t imm_rvalue = ((operator == UN_PREINC) || (operator == UN_POSTINC)) ? builder->value_one : builder->value_minus_one;

	if (is_prefix)
	{
		ir_emit_binary_operation(enc, operand_rvalue, operand_rvalue, imm_rvalue, BIN_ADD);
		ir_build_iadd(builder, operand_value);
		ir_build_store(builder, operand_lvalue, operand_rvalue);
		return operand_value;
	}
	else
	{
		const ir_value* result_value = ir_build_temp(builder);

		//ir_emit_binary_operation(enc, &post_result_rvalue, &operand_rvalue, &imm_rvalue, BIN_ADD);
		ir_build_store();
		const item_t incremented_value = ir_build_iadd(builder, operand_value, imm_value, post_result_value);
		ir_build_store(builder, operand_value, post_result_value);
		ir_free_value(enc, incremented value);
	}
}

static ir_value* ir_emit_unary_expression(ir_builder *const builder, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);
	assert(operator != UN_INDIRECTION);

	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			return ir_emit_increment_expression(builder, nd);

		case UN_MINUS:
		case UN_NOT:
		{
			const node operand = expression_unary_get_operand(nd);
			const ir_value *const operand_rvalue = ir_emit_expression(builder, &operand);
			const binary_t instruction = (operator == UN_MINUS) ? BIN_MUL : BIN_XOR;

			ir_emit_binary_operation(enc, &operand_rvalue, &operand_rvalue, &RVALUE_NEGATIVE_ONE, instruction);
			return operand_rvalue;
		}

		case UN_LOGNOT:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue value = emit_expression(enc, &operand);

			to_code_2R_I(enc->sx->io, IC_MIPS_SLTIU, value.val.reg_num, value.val.reg_num, 1);
			return value;
		}

		case UN_ABS:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue operand_rvalue = emit_expression(enc, &operand);
			const mips_instruction_t instruction = type_is_floating(operand_rvalue.type) ? IC_MIPS_ABS_S : IC_MIPS_ABS;

			to_code_2R(enc->sx->io, instruction, operand_rvalue.val.reg_num, operand_rvalue.val.reg_num);
			return operand_rvalue;
		}

		case UN_ADDRESS:
		{
			const node operand = expression_unary_get_operand(nd);
			const lvalue operand_lvalue = emit_lvalue(enc, &operand);

			assert(operand_lvalue.kind != LVALUE_KIND_REGISTER);

			const rvalue result_rvalue = {
				.from_lvalue = !FROM_LVALUE,
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = get_register(enc),
				.type = TYPE_INTEGER
			};

			to_code_2R_I(
				enc->sx->io,
				IC_MIPS_ADDI,
				result_rvalue.val.reg_num,
				operand_lvalue.base_reg,
				operand_lvalue.loc.displ
			);
			return result_rvalue;
		}

		case UN_UPB:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue array_address = emit_expression(enc, &operand);
			const lvalue size_lvalue = {
				.base_reg = array_address.val.reg_num,
				.kind = LVALUE_KIND_STACK,
				.loc.displ = WORD_LENGTH,
				.type = TYPE_INTEGER
			};
			return emit_load_of_lvalue(enc, &size_lvalue);
		}

		default:
			system_error(node_unexpected);
			return builder->value_void;
	}
}

static item_t ir_emit_binary_operation(ir_builder *const builder, const item_t lhs, const item_t rhs, const binary_t operator)
{
	assert(operator != BIN_LOG_AND);
	assert(operator != BIN_LOG_OR);

	// assert(dest->kind == RVALUE_KIND_REGISTER);
	// assert(first_operand->kind != RVALUE_KIND_VOID);
	// assert(second_operand->kind != RVALUE_KIND_VOID);

	// Stub.
	return 0;
}

static item_t ir_emit_binary_expression(ir_builder *const builder, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const node lhs = expression_binary_get_LHS(nd);
	const node rhs = expression_binary_get_RHS(nd);

	// Stub.
	return 0;
}

static item_t ir_emit_ternary_expression(ir_builder *const builder, const node *const nd)
{
	const node condition = expression_ternary_get_condition(nd);
	const node lhs = expression_ternary_get_LHS(nd);
	const node rhs = expression_ternary_get_RHS(nd);

	const item_t condition_value = ir_emit_expression(condition);
	const item_t lhs_value = ir_emit_expression(builder, lhs);
	const item_t rhs_value = ir_emit_expression(builder, rhs); 

	const item_t res_value = ir_build_select(builder, lhs_value, rhs_value, condition_value);

	ir_free_value(condition_value);
	ir_free_value(lhs_value);
	ir_free_value(rhs_value);

	return res_value;
}

static ir_value* ir_emit_struct_assignment(encoder *const enc, const lvalue *const target, const node *const value)
{
	if (expression_get_class(value) == EXPR_INITIALIZER)  // Присваивание списком
	{
		emit_structure_init(enc, target, value);
	}
	else  // Присваивание другой структуры
	{
		// FIXME: возврат структуры из функции
		// FIXME: массив структур
		const size_t RHS_identifier = expression_identifier_get_id(value);
		const lvalue RHS_lvalue = displacements_get(enc, RHS_identifier);

		// Копирование всех данных из RHS
		const item_t type = expression_get_type(value);
		const size_t struct_size = mips_type_size(enc->sx, type);
		for (size_t i = 0; i < struct_size; i += WORD_LENGTH)
		{
			// Грузим данные из RHS
			const lvalue value_word = {
				.base_reg = RHS_lvalue.base_reg,
				.loc.displ = RHS_lvalue.loc.displ + i,
				.kind = LVALUE_KIND_STACK,
				.type = TYPE_INTEGER
			};
			const rvalue proxy = emit_load_of_lvalue(enc, &value_word);

			// Отправляем их в variable
			const lvalue target_word = {
				.base_reg = target->base_reg,
				.kind = LVALUE_KIND_STACK,
				.loc.displ = target->loc.displ + i,
				.type = TYPE_INTEGER
			};
			emit_store_of_rvalue(enc, &target_word, &proxy);

			free_rvalue(enc, &proxy);
		}
	}

	return emit_load_of_lvalue(enc, target);
}

static ir_value* ir_emit_assignment_expression(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const node lhs = expression_assignment_get_LHS(nd);
	const item_t target = ir_emit_lvalue(builder, &lhs);

	const node rhs = expression_assignment_get_RHS(nd);
	const item_t rhs_type = expression_get_type(&rhs);

	if (type_is_structure(enc->sx, RHS_type))
	{
		return ir_emit_struct_assignment(enc, &target, &RHS);
	}

	const rvalue value = emit_expression(enc, &RHS);

	const binary_t operator = expression_assignment_get_operator(nd);
	if (operator == BIN_ASSIGN)
	{
		emit_store_of_rvalue(enc, &target, &value);
		return value;
	}

	// это "+=", "-=" и т.п.
	const rvalue target_value = emit_load_of_lvalue(enc, &target);
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
			return RVALUE_VOID;
	}

	emit_binary_operation(enc, &target_value, &target_value, &value, correct_operation);
	free_rvalue(enc, &value);

	emit_store_of_rvalue(enc, &target, &target_value);
	return target_value;
}

static item_t ir_emit_expression(ir_builder *const builder, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		const item_t value = ir_emit_lvalue(builder, nd);
		return ir_emit_load(builder, value);
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
			return RVALUE_VOID;

		default:
			system_error(node_unexpected);
			return RVALUE_VOID;
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
		ir_free_rvalue(enc, &result);
	}
	return IR_VALUE_VOID;
}


//
// Генерация объявлений.
//


static void ir_emit_array_init(encoder *const enc, const node *const nd, const size_t dimension, 
	const node *const init, const rvalue *const addr)
{
}

static rvalue ir_emit_bound(encoder *const enc, const node *const bound, const node *const nd)
{

}


static void ir_emit_array_declaration(encoder *const enc, const node *const nd)
{
}

static void ir_emit_structure_init(encoder *const enc, const lvalue *const target, const node *const initializer)
{
	assert(type_is_structure(enc->sx, target->type));

	size_t displ = 0;

	const size_t amount = type_structure_get_member_amount(enc->sx, target->type);
	for (size_t i = 0; i < amount; i++)
	{
		const item_t type = type_structure_get_member_type(enc->sx, target->type, i);
		const lvalue member_lvalue = {
			.base_reg = target->base_reg,
			.kind = target->kind,
			.loc.displ = target->loc.displ + displ,
			.type = type
		};
		displ += mips_type_size(enc->sx, type);

		const node subexpr = expression_initializer_get_subexpr(initializer, i);
		if (expression_get_class(&subexpr) == EXPR_INITIALIZER)
		{
			emit_structure_init(enc, &member_lvalue, &subexpr);
			continue;
		}

		const rvalue subexpr_rvalue = emit_expression(enc, &subexpr);
		emit_store_of_rvalue(enc, &member_lvalue, &subexpr_rvalue);
	}
}

static void ir_emit_variable_declaration(ir_builder *const builder, const node *const nd)
{
	const syntax *const sx = builder->sx;

	const size_t identifier = declaration_variable_get_id(nd);
	const item_t type = ident_get_type(sx, identifier);

	const item_t var_ptr_value = ir_build_alloca(builder, type);

	displacements_add(
		builder,
		identifier,
		var_ptr_value
	);

	if (declaration_variable_has_initializer(nd))
	{
		const node initializer = declaration_variable_get_initializer(nd);

		if (type_is_structure(sx, type))
		{
			const item_t struct_value = ir_emit_struct_assignment(enc, &variable, &initializer);
			ir_free_value(builder, struct_value);
		}
		else
		{
			const item_t value = ir_emit_expression(enc, &initializer);

			ir_build_store(enc, var_ptr_value, value);
			ir_free_value(enc, value);
		}
	}
}

static void ir_emit_function_definition(ir_builder *const builder, const node *const nd)
{
	const size_t ref_ident = declaration_function_get_id(nd);

	ir_build_function_def(builder, item_t name, );

	const item_t func_type = ident_get_type(enc->sx, ref_ident);
	const size_t parameter_count = type_function_get_parameter_amount(enc->sx, func_type);

	node body = declaration_function_get_body(nd);
	ir_emit_statement(enc, &body);

	ir_build_ret();
}

static void ir_emit_declaration(ir_builder *const builder, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			ir_emit_variable_declaration(enc, nd);
			break;

		case DECL_FUNC:
			ir_emit_function_definition(enc, nd);
			break;

		default:
			// С объявлением типа ничего делать не нужно
			return;
	}

	uni_printf(enc->sx->io, "\n");
}

//
// Генерация IR из выражений
//

static void ir_emit_statement(ir_builder *const builder, const node *const nd);

static void ir_emit_declaration_statement(ir_builder *const builder, const node *const nd)
{
	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		emit_declaration(enc, &decl);
	}
}

static void ir_emit_case_statement(ir_builder *const builder, const node *const nd, const size_t label_num)
{
	const label label_case = { .kind = L_CASE, .num = label_num };
	emit_label_declaration(enc, &label_case);

	const node substmt = statement_case_get_substmt(nd);
	emit_statement(enc, &substmt);
}

static void ir_emit_default_statement(ir_builder *const builder, const node *const nd, const size_t label_num)
{
	const label label_default = { .kind = L_CASE, .num = label_num };
	ir_emit_label_declaration(enc, &label_default);

	const node substmt = statement_default_get_substmt(nd);
	ir_emit_statement(enc, &substmt);
}

static void ir_emit_compound_statement(ir_builder *const builder, const node *const nd)
{
	const size_t scope_displacement = enc->scope_displ;

	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}

	enc->max_displ = max(enc->scope_displ, enc->max_displ);
	enc->scope_displ = scope_displacement;
}

static void ir_emit_switch_statement(ir_builder *const builder, const node *const nd)
{
	const size_t label_num = enc->label_num++;
	size_t curr_case_label_num = enc->case_label_num;

	const label old_label_break = enc->label_break;
	enc->label_break = (label){ .kind = L_END, .num = label_num };

	const node condition = statement_switch_get_condition(nd);
	const rvalue tmp_condtion = emit_expression(enc, &condition);
	const rvalue condition_rvalue = (tmp_condtion.kind == RVALUE_KIND_CONST)
		? emit_load_of_immediate(enc, &tmp_condtion)
		: tmp_condtion;

	item_t default_index = -1;

	// Размещение меток согласно условиям
	const node body = statement_switch_get_body(nd);
	const size_t amount = statement_compound_get_size(&body); // Гарантируется compound statement
	for (size_t i = 0; i < amount; i++)
	{
		const node substmt = statement_compound_get_substmt(&body, i);
		const item_t substmt_class = statement_get_class(&substmt);

		if (substmt_class == STMT_CASE)
		{
			const size_t case_num = enc->case_label_num++;
			const label label_case = { .kind = L_CASE, .num = case_num };

			const node case_expr = statement_case_get_expression(&substmt);
			const rvalue case_expr_rvalue = emit_literal_expression(enc, &case_expr);

			// Пользуемся тем, что это integer type
			const rvalue result_rvalue = {
				.from_lvalue = !FROM_LVALUE,
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = get_register(enc),
				.type = TYPE_INTEGER
			};
			emit_binary_operation(enc, &result_rvalue, &condition_rvalue, &case_expr_rvalue, BIN_EQ);
			emit_conditional_branch(enc, IC_MIPS_BEQ, &result_rvalue, &label_case);

			free_rvalue(enc, &result_rvalue);
		}
		else if (substmt_class == STMT_DEFAULT)
		{
			// Только получаем индекс, а размещение прыжка по нужной метке будет после всех case'ов
			default_index = enc->case_label_num++;
		}
	}

	if (default_index != -1)
	{
		const ir_label label_default = { .kind = L_CASE, .num = (size_t)default_index };
		ir_emit_jump(enc, IC_MIPS_J, &label_default);
	}
	else
	{
		// Нет default => можем попасть в ситуацию, когда требуется пропустить все case'ы
		ir_emit_jump(builder, IC_MIPS_J, &enc->label_break);
	}

	ir_free_ralue(builder, &condition_rvalue);

	// Размещение тел всех case и default statements
	for (size_t i = 0; i < amount; i++)
	{
		const node substmt = statement_compound_get_substmt(&body, i);
		const item_t substmt_class = statement_get_class(&substmt);

		if (substmt_class == STMT_CASE)
		{
			ir_emit_case_statement(builder, &substmt, curr_case_label_num++);
		}
		else if (substmt_class == STMT_DEFAULT)
		{
			ir_emit_default_statement(builder, &substmt, curr_case_label_num++);
		}
		else
		{
			ir_emit_statement(builder, &substmt);
		}
	}

	ir_emit_label(builder, &enc->label_break);
	enc->label_break = old_label_break;
}

static void ir_emit_if_statement(ir_builder *const builder, const node *const nd) 
{
	const node condition = statement_if_get_condition(nd);
	const ir_value* value = ir_emit_expression(enc, &condition);

	const ir_label label_else = create_ir_label(L_ELSE);//ir_build_label(ctx, L_ELSE);
	const ir_label label_end = create_ir_label(L_END);//ir_build_label(ctx, L_END);
	const bool has_else = statement_if_has_else_substmt(nd);

	ir_emit_jump_cond(enc, instruction, &value, has_else ? &label_else : &label_end);
	ir_free_value(enc, &value);

	const node then_substmt = statement_if_get_then_substmt(nd);
	emit_statement(enc, &then_substmt);

	if (has_else)
	{
		ir_emit_jump(enc, &label_end);
		ir_emit_label(enc, &label_else);

		const node else_substmt = statement_if_get_else_substmt(nd);
		ir_emit_statement(enc, &else_substmt);
	}

	ir_emit_label(enc, &label_end);
}

static void ir_emit_continue_statement(ir_builder *const builder)
{
	ir_build_jump(builder, builder->label_continue);
}

static void ir_emit_break_statement(ir_builder *const builder)
{
	ir_build_jump(builder, builder->label_break);
}

static void ir_emit_variable_declaration(ir_builder *const builder, const node *const nd)
{
	const size_t identifier = declaration_variable_get_id(nd);

	const item_t type = ident_get_type(builder->sx, identifier);
	if (type_is_array(enc->sx, type))
	{
		emit_array_declaration(enc, nd);
	}
	else
	{
		const bool is_register = false; // Тут должно быть что-то типо ident_is_register (если не откажемся)
		const lvalue variable = displacements_add(
			enc,
			identifier,
			// что-то такое, как будет поддержано: (is_register) ? <получение регистра> : info->scope_displ,
			enc->scope_displ,
			is_register
		);
		if (declaration_variable_has_initializer(nd))
		{
			const node initializer = declaration_variable_get_initializer(nd);

			if (type_is_structure(enc->sx, type))
			{
				const rvalue tmp = ir_emit_struct_assignment(builder, &variable, &initializer);
				ir_free_value(builder, &tmp);
			}
			else
			{
				const ir_value* value = ir_emit_expression(builder, &initializer);

				ir_emit_store_of_rvalue(builder, &variable, &value);
				ir_free_value(builder, &value);
			}
		}
	}
}

static void ir_emit_while_statement(ir_builder *const builder, const node *const nd)
{
	const ir_label label_begin = create_ir_label(L_BEGIN_CYCLE); // { .kind = L_BEGIN_CYCLE, .num = label_num };
	const ir_label label_end = create_ir_label(L_END); // { .kind = L_END, .num = label_num };

	const ir_label old_continue = enc->label_continue;
	const ir_label old_break = enc->label_break;

	enc->label_continue = label_begin;
	enc->label_break = label_end;

	emit_label_declaration(enc, &label_begin);

	const node condition = statement_while_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const mips_instruction_t instruction = IC_MIPS_BEQ;
	emit_conditional_branch(enc, instruction, &value, &label_end);
	ir_free_value(enc, &value);

	const node body = statement_while_get_body(nd);
	ir_emit_statement(builder, &body);

	ir_build_jump(builder, &label_begin);
	ir_build_label(builder, &label_end);

	enc->label_continue = old_continue;
	enc->label_break = old_break;
}

static void ir_emit_do_statement(encoder *const enc, const node *const nd)
{
	const size_t label_num = enc->label_num++;
	const label label_begin = { .kind = L_BEGIN_CYCLE, .num = label_num };
	emit_label_declaration(enc, &label_begin);

	const label label_condition = { .kind = L_NEXT, .num = label_num };
	const label label_end = { .kind = L_END, .num = label_num };

	const label old_continue = enc->label_continue;
	const label old_break = enc->label_break;
	enc->label_continue = label_condition;
	enc->label_break = label_end;

	const node body = statement_do_get_body(nd);
	emit_statement(enc, &body);
	emit_label_declaration(enc, &label_condition);

	const node condition = statement_do_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const mips_instruction_t instruction = IC_MIPS_BNE;
	emit_conditional_branch(enc, instruction, &value, &label_begin);
	emit_label_declaration(enc, &label_end);
	free_rvalue(enc, &value);

	enc->label_continue = old_continue;
	enc->label_break = old_break;
}

static void ir_emit_for_statement(ir_builder *const builder, const node *const nd)
{
	const size_t scope_displacement = enc->scope_displ;

	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		ir_emit_statement(enc, &inition);
	}

	const size_t label_num = enc->label_num++;
	const ir_label label_begin = create_ir_label(L_BEGIN_CYCLE);
	const ir_label label_end = create_ir_label(L_END);

	const ir_label *const old_continue = context->label_continue;
	const ir_label *const old_break = context->label_break;
	builder->label_continue = &label_begin;
	builder->label_break = &label_end;

	ir_build_label(enc, &label_begin);
	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		const ir_value* value = ir_emit_expression(builder, &condition);
		ir_build_jump_cond(builder, value, label_end);
		ir_free_value(builder, &value);
	}

	const node body = statement_for_get_body(nd);
	ir_emit_statement(enc, &body);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		ir_emit_increment(context, &increment);
	}

	ir_build_jump(context, &label_begin);
	ir_build_label(context, &label_end);

	builder->label_continue = old_continue;
	builder->label_break = old_break;

	//builder->scope_displ = scope_displacement;
}

static void ir_emit_return_statement(ir_builder *const builder, const node *const nd)
{
	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		const rvalue value = ir_emit_expression(enc, &expression);

		const lvalue return_lval = { .kind = LVALUE_KIND_REGISTER, .loc.reg_num = R_V0, .type = value.type };

		emit_store_of_rvalue(enc, &return_lval, &value);
		free_rvalue(enc, &value);
	}

	const label label_end = { .kind = L_FUNCEND, .num = enc->curr_function_ident };
	emit_unconditional_branch(enc, IC_MIPS_J, &label_end);
	ir_build_ret(builder, );
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
			ir_emit_continue_statement(context);
			break;

		case STMT_BREAK:
			ir_emit_break_statement(context);
			break;

		case STMT_RETURN:
			ir_emit_return_statement(context, nd);
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

static void ir_eval_instr(ir_context* ctx, ir_instr* instr)
{
	eval_value(inst->op1);
}
static void ir_eval_function(ir_context *const ctx, ir_function *const function)
{
	ctx->evals->emit_function_pre();
	for (size_t i = 0; i < ir_function_get_instr_count(function); i++)
	{
		ir_eval_instr(ctx, );
	}
	ctx->evals->emit_function_post();

	for (size_t)
}
static void ir_eval_extern(ir_context *const ctx, const ir_extern *const extern_)
{
	ctx->evals->emit_extern(NULL, extern_);
}
static void ir_eval_bin(ir_context *const ctx, const ir_instr *const instr)
{
	switch (ir_instr_get_ic(instr))
	{

	}
}
static void ir_eval_instr(ir_context *const ctx, const ir_instr *const instr)
{
	switch (ir_instr_get_ic())
	{
		case IR_IC_NOP:
			ir_eval_nop(ctx, instr);
		case IR_IC_ALLOCA:
			ir_eval_alloca(ctx, instr);
		case IR_IC_LABEL:
			ir_eval_label(ctx, instr);

		case IR_IC_LOAD:
			ir_eval_load(ctx, instr);
		case IR_IC_STORE:
			ir_eval_store(ctx, instr);

		case IR_IC_ADD:
		case IR_IC_SUB:
		case IR_IC_MUL:
		case IR_IC_DIV:
			ir_eval_bin(ctx, instr);

		case IR_IC_FADD:
		case IR_IC_FSUB:
		case IR_IC_FMUL:
		case IR_IC_FDIV:
			ir_eval_fbin(ctx, instr);
	}
}
static void ir_eval_module(const ir_module *const module, ir_evals* evals)
{
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

