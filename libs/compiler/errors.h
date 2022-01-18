/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
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

#pragma once

#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Errors codes */
typedef enum ERROR
{
	// Lexing errors
	bad_character,							/**< Bad character */
	digit_of_another_base,					/**< Digit of another base */
	exponent_has_no_digits,					/**< Exponent has no digits */
	empty_character_literal,				/**< Empty character literal */
	unknown_escape_sequence,				/**< Unknown escape sequence */
	missing_terminating_apost_char,			/**< Missing terminating ' character */
	missing_terminating_quote_char,			/**< Missing terminating " character */
	unterminated_block_comment,				/**< Unterminated block comment */

	// Syntax errors
	expected_r_paren,						/**< Expected ')' */
	expected_r_square,						/**< Expected ']' */
	expected_r_brace,						/**< Expected '}' */
	expected_identifier_in_member_expr,		/**< Expected identifier in member expression */
	expected_colon_in_conditional_expr,		/**< Expected ':' in conditional expression */
	empty_initializer,						/**< Empty initializer */
	expected_l_paren_in_condition,			/**< Expected '(' in condition */
	case_not_in_switch,						/**< 'case' statement not in switch statement */
	default_not_in_switch,					/**< 'default' statement not in switch statement */
	expected_colon_after_case,				/**< Expected ':' after 'case' */
	expected_colon_after_default,			/**< Expected ':' after 'default' */
	expected_semi_after_expr,				/**< Expected ';' after expression */
	expected_semi_after_stmt,				/**< Expected ';' after statement */
	expected_while,							/**< Expected 'while' in do/while loop */
	expected_paren_after_for,				/**< Expected '(' after 'for' */
	expected_semi_in_for,					/**< Expected ';' in for */
	continue_not_in_loop,					/**< 'continue' statement not in loop statement */
	break_not_in_loop_or_switch,			/**< 'break' statement not in loop or switch statement */

	// Semantics errors
	undeclared_identifier_use,				/**< Use of undeclared identifier */
	subscripted_expr_not_array,				/**< Subscripted expression is not an array */
	array_subscript_not_integer,			/**< Array subscript is not an integer */
	called_expr_not_function,				/**< Called expression is not a function */
	wrong_argument_amount,					/**< Wrong argument amount in call expression */
	member_reference_not_struct,			/**< Member reference base type is not a structure */
	member_reference_not_struct_pointer,	/**< Member reference type is not a structure pointer */
	no_such_member,							/**< No such member */
	illegal_increment_type,					/**< Cannot increment/decrement value of that type */
	unassignable_expression,				/**< Expression is not assignable */
	cannot_take_rvalue_address,				/**< Cannot take the address of an rvalue */
	indirection_requires_pointer,			/**< Indirection requires pointer operand */
	typecheck_unary_expr,					/**< Invalid argument type to unary expression */
	typecheck_binary_expr,					/**< Invalid argument type to binary expression */
	condition_must_be_scalar,				/**< Condition must be of scalar type */
	too_many_printf_args,					/**< Too many printf arguments */
	expected_format_specifier,				/**< Expected format specifier */
	unknown_format_specifier,				/**< Unknown format specifier */
	printf_fst_not_string,					/**< First argument of 'printf' call is not a string literal */
	wrong_printf_argument_amount,			/**< Wrong argument amount in 'printf' call */
	wrong_printf_argument_type,				/**< Wrong argument type in 'printf' call */
	pointer_in_print,						/**< Pointer in print */
	expected_identifier_in_printid,			/**< Expected identifier in printid */
	expected_identifier_in_getid,			/**< Expected identifier in getid */
	upb_operand_not_array,					/**< Operand of 'upb' is not an array */
	expected_constant_expression,			/**< Expected constant expression */
	incompatible_cond_operands,				/**< Incompatible operand types in conditional expression */
	case_expr_not_integer,					/**< Case expression is not an integer */
	switch_expr_not_integer,				/**< Switch expression is not an integer */
	void_func_valued_return,				/**< Void function should not return a value */
	nonvoid_func_void_return,				/**< Non-void function should return a value */
	bad_type_in_ret,

	// Environment errors
	no_main_in_program,						/**< Undefined main */
	predef_but_notdef,						/**< Undefined function */

	// Other errors
	after_type_must_be_ident = 201,
	empty_struct,
	empty_enum,
	wait_right_sq_br,
	only_functions_may_have_type_VOID,
	decl_and_def_have_diff_type,
	wrong_param_list,
	expected_semi_after_decl,
	typedef_requires_a_name,
	func_decl_req_params,
	cond_must_be_in_brkts,
	repeated_decl,
	arr_init_must_start_from_BEGIN,
	no_comma_in_init_list,
	ident_is_not_declared,
	no_rightsqbr_in_slice,
	index_must_be_int,
	slice_not_from_array,
	call_not_from_function,
	no_comma_in_act_params,
	float_instead_int,
	wait_rightbr_in_primary,
	unassignable_inc,
	wrong_addr,
	no_colon_in_cond_expr,
	int_op_for_float,
	not_const_int_expr,
	assmnt_float_to_int,
	redefinition_of_main,
	init_int_by_float,
	no_comma_in_setmotor,
	param_setmotor_not_int,
	expected_end,
	aster_before_func,
	aster_not_for_pointer,
	aster_with_row,
	float_in_condition,
	wrong_func_as_arg,
	no_right_br_in_arg_func,
	par_type_void_with_nofun,
	ident_in_declarator,
	array_before_func,
	wait_definition,
	wait_declarator,
	two_idents_for_1_declarer,
	function_has_no_body,
	diff_formal_param_type_and_actual,
	expected_expression,
	wrong_operand,
	operand_is_pointer,
	wrong_struct,
	after_dot_must_be_ident,
	get_field_not_from_struct_pointer,
	error_in_initialization,
	error_in_equal_with_enum,
	type_missmatch,
	array_assigment,
	wrong_struct_ass,
	wrong_init,
	no_field,
	slice_from_func,
	wait_end,
	act_param_not_ident,
	unassignable,
	pnt_before_array,
	array_size_must_be_int,
	no_semicolon_in_struct,
	no_comma_in_enum,
	wait_ident_after_semicolon_in_struct,
	wait_ident_after_comma_in_enum,
	no_equal_with_enum,
	empty_init,
	ident_not_type,
	not_decl,
	print_without_br,
	select_not_from_struct,
	init_not_struct,
	param_threads_not_int,

	empty_bound_without_init,
	begin_with_notarray,
	string_and_notstring,
	wrong_init_in_actparam,
	no_comma_or_end,

	struct_init_must_start_from_BEGIN,

	// Tree parsing errors
	tree_expression_not_block,
	tree_expression_unknown,
	tree_expression_operator,
	tree_expression_no_texprend,

	// Tree testing errors
	tree_no_tend,
	tree_unexpected,

	node_cannot_add_child,
	node_cannot_set_type,
	node_cannot_add_arg,
	node_unexpected,

	// Codegen errors
	tables_cannot_be_compressed,
} error_t;

/** Warnings codes */
typedef enum WARNING
{
	too_long_int,
	variable_deviation,

	tree_operator_unknown,
	node_argc,
} warning_t;


/**
 *	Emit an error for some problem
 *
 *	@param	io			Universal io
 *	@param	num			Error number
 */
void error(const universal_io *const io, error_t num, ...);

/**
 *	Emit a warning for some problem
 *
 *	@param	io			Universal io
 *	@param	num			Warning number
 */
void warning(const universal_io *const io, warning_t num, ...);


/**
 *	Emit an error (embedded version)
 *
 *	@param	io			Universal io
 *	@param	num			Error number
 *	@param	args		Variable list
 */
void verror(const universal_io *const io, const error_t num, va_list args);

/**
 *	Emit a warning (embedded version)
 *
 *	@param	io			Universal io
 *	@param	num			Warning number
 *	@param	args		Variable list
 */
void vwarning(const universal_io *const io, const warning_t num, va_list args);


/**
 *	Emit an error by number
 *
 *	@param	num			Error number
 */
void system_error(error_t num, ...);

/**
 *	Emit a warning by number
 *
 *	@param	num			Warning number
 */
void system_warning(warning_t num, ...);


/**
 *	Emit an error message
 *
 *	@param	msg			Error message
 */
void error_msg(const char *const msg);

/**
 *	Emit a warning message
 *
 *	@param	msg			Warning message
 */
void warning_msg(const char *const msg);

#ifdef __cplusplus
} /* extern "C" */
#endif
