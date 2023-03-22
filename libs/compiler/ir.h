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

#pragma once

#include <stdlib.h>
#include <ctype.h>
#include "AST.h"
#include "hash.h"
#include "node_vector.h"
#include "operations.h"
#include "syntax.h"
#include "tree.h"
#include "syntax.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum rvalue_kind
{
	RVALUE_KIND_TEMP,
	RVALUE_KIND_IMM
} rvalue_kind;

/**
 *	@~english
 *	@brief Rvalues are passed into instruction generation functions for a specific
 *	platform. Rvalue is a value, that can be calculated, but assignment operation
 *	can't be applied to it, because it could not be stored in memory.
 *
 *	@~russian
 *	@brief Rvalue передается в реализации функций генерации для конкретной платформы.
 *	Rvalue - значение, которое можно вычислить, но применить операцию присваивания
 *	к нему нельзя, потому что оно может находиться не в памяти.
 */
typedef struct rvalue
{
	rvalue_kind kind;
	size_t id;
	union 
	{
		char* string;
		float float_;
		int int_;
	} value;
} rvalue;

typedef enum lvalue_kind
{
	LVALUE_KIND_GLOBAL,
	LVALUE_KIND_LOCAL,
	LVALUE_KIND_PARAM
} lvalue_kind;

/**
 *	@~english
 *	@brief Lvalues are passed into instruction generation functions for a specific
 *	platform. Lvalue is a value, that has a place in a memory, so the assingment
 *	operation can be applied to it.
 *
 *	@~russian
 *	@brief Lvalue передается в реализации функций генерации для конкретной платформы.
 *	Lvalue - значение, имеющее область в памяти, то есть такое, к которому можно
 *	применить операции присвоения.
 */
typedef struct lvalue
{
	lvalue_kind kind;
} lvalue;

/**
 *	@~english
 *	@brief IR evals is a set of function pointers that are called during a tree
 *	evaluation. The function implementations are individual to each platform.
 *	It is some kind of a "visitor" GoF pattern.
 *
 *	@~russian
 *	@brief Набор указателей на функции которые вызываются во время выполнения
 *	дерева. Реализация функций индивидуальна для каждой платформы. Что-то вроде
 *	паттерна "посетитель".
 */
typedef struct ir_evals
{
	void (*emit_const_int)(void *const ctx, int value);
	void (*emit_const_float)(void *const ctx, float value);
	void (*emit_const_string)(void *const ctx, char* value);
	void (*emit_rvalue)(void *const ctx, const rvalue *const value);
	void (*emit_lvalue)(void *const ctx, const lvalue *const value);

	void (*emit_store)(void *const ctx, const rvalue *const value, const rvalue *const dest);
	void (*emit_load)(void *const ctx, const rvalue *const source);
	void (*emit_alloca)(void *const ctx, size_t size);

	void (*emit_iadd)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_isub)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_imul)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_idiv)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);

	void (*emit_fadd)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_fsub)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_fmul)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);
	void (*emit_fiv)(void *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res);

	void (*emit_label)(void *const ctx, const char* name);
} ir_evals;

/**
 *	@~english
 *	@brief An IR module is a set of functions, global variables and externs. It also
 *	includes values, used in function instructions. A single module is a single
 *	translation unit.
 *
 *	@~russian
 *	@brief IR модуль - набор функций, глобальных перменных и extern-ов. Так же содержит 
 *	значения, используемые в инструкциях функций. Фактически эквивалентен единице 
 *	трансляции.
 */
typedef struct ir_module
{
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
} ir_module;

#define IR_MAX_TEMP_VALUES 8

/**
 *	@~english
 *	@brief An IR builder is some kind of context for a specific module, used to 
 *	build an IR tree.
 *
 *	@~russian
 *	@brief Построитель IR дерева, что-то вроде контекста для определенного модуля,
 *	используется при построении IR дерева.
 */
typedef struct ir_builder
{
	const syntax *const sx;

	ir_module* module;

	item_t temp_values[IR_MAX_TEMP_VALUES];
	bool temp_used[IR_MAX_TEMP_VALUES];

	hash displs;

	item_t function;

	item_t break_label;
	item_t continue_label;
	item_t function_end_label;

	item_t value_zero;
	item_t value_fzero;
	item_t value_one;
	item_t value_minus_one;
	item_t value_fone;
	item_t value_fminus_one;
} ir_builder;

/**
 *	@~english
 *	@brief An IR context, used during a tree evaluation for generating code.
 *
 *	@~russian
 *	@brief IR контекст, используемый при выполнении IR дерева для генерации кода.
 */
typedef struct ir_context ir_context;

/**
 *	@related ir_module
 *
 *	@~russian
 *	@brief Создать новый пустой IR модуль.
 *
 *	@~english
 *	@brief Create a new empty IR module.
 */
ir_module create_ir_module();
/**
 *	@~russian
 *	@brief Создать новый построитель IR дерева для заданного модуля, используется 
 *	после построения абстрактного дерева.
 *
 *	@~english
 *	@brief Create a new IR tree builder for a specific module, used after 
 *	bulding an abstract syntax tree.
 */
ir_builder create_ir_builder(ir_module *const module, const syntax *const sx);

/**
 *	@memberof ir_builder
 *
 *	@~english
 *	@brief Dumps IR module in format of special IR language to stdout.
 *
 *	@~russian
 *	@brief Выводит модуль в формате специального IR языка в stdout.
 */
void ir_dump(const ir_builder *const builder, const ir_module *const module);
/**
 *	@memberof ir_builder
 *
 *	@~english
 *	@brief 
 *
 *	@param builder IR Builder.
 *	@param nd Abstract syntax tree root.
 *
 *	@~russian
 *	@brief Обойти абстрактное синтаксическое дерево и построить исходя из него
 *	IR дерево.
 *
 *	@param builder Построитель IR.
 *	@param nd Корень абстрактного синтаксического дерева.
 */
void ir_emit_module(ir_builder *const builder, const node *const nd);
/**
 *	@memberof ir_module
 *
 *	@~english 
 *	@brief IR module and generate platform-dependent code using functions
 *	specified in evals. Used when generating code for a specific architecture.
 *
 *	@~russian
 *	@brief Обходит IR модуль и генерирует платформозависимый код при помощи 
 *	функций из evals. Используется при генерации кода для конкретной архитектуры.
 */
void ir_eval_module(const ir_module *const module, const ir_evals *const evals);

#ifdef __cplusplus
}
#endif