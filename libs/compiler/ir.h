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

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ir_evals
{
	size_t int_size;
	size_t float_size;
	size_t bool_size;
	size_t char_size;
	size_t pointer_size;

	void (*emit_rvalue)(void *const ctx, const rvalue *const value);
	void (*emit_lvalue)(void *const ctx, const lvalue *const value);
	void (*emit_const_int)(void *const ctx, int value);
	void (*emit_const_float)(void *const ctx, float value);
	void (*emit_const_string)(void *const ctx, char* value);
	void (*emit_store)(void *const context, ir_value* value, ir_value* dest);
	void (*emit_load)(void *const context, ir_value* source);
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

typedef enum rvalue_kind
{
	RVALUE_KIND_TEMP,
	RVALUE_KIND_CONST
} rvalue_kind;

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

typedef struct lvalue_kind
{
	LVALUE_KIND_GLOBAL,
	LVALUE_KIND_LOCAL,
	LVALUE_KIND_PARAM
} lvalue_kind;

struct ir_module;
typedef struct ir_module ir_module;

struct ir_builder;
typedef struct ir_builder;

ir_module create_ir_module();
void ir_emit_module(ir_builder *builder, ir_module *module);
void ir_eval_module(ir_context *context, ir_module *module);

#ifdef __cplusplus
}
#endif