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
	// Lexer errors
	bad_character,							/**< Bad character in source */
	empty_character,						/**< Empty character constant */
	unknown_escape_sequence,				/**< Unknown escape sequence */
	expected_apost_after_char_const,		/**< Missing terminating ' character */
	missing_terminating_quote_char,			/**< Missing terminating '"' character */
	unterminated_block_comment,				/**< Unterminated block comment */

	// Expression errors
	undeclared_var_use,						/**< Use of undeclared identifier */
	expected_r_paren,						/**< Expected ')' */
	typecheck_subscript_value,				/**< Subscripted value is not an array */
	typecheck_subscript_not_integer,		/**< Array subscript is not an integer */
	expected_r_square,						/**< Expected ']' */
	expected_r_brace,						/**< Expected '}' */
	expected_identifier,					/**< Expected identifier */
	typecheck_call_not_function,			/**< Called object type is not a function */
	typecheck_convert_incompatible,			/**< Passing type to parameter of incompatible type */
	typecheck_member_reference_struct,		/**< Member reference base type is not a structure */
	typecheck_member_reference_arrow,		/**< Member reference type is not a pointer */
	typecheck_member_reference_ivar,		/**< Struct does not have a member named that */
	no_member,								/**< No member named that */
	typecheck_illegal_increment,			/**< Cannot increment/decrement value of that type */
	typecheck_expression_not_lvalue,		/**< Expression is not assignable */
	typecheck_invalid_lvalue_addrof,		/**< Cannot take the address of an rvalue */
	typecheck_indirection_requires_pointer,	/**< Indirection requires pointer operand */
	typecheck_unary_expr,					/**< Invalid argument type to unary expression */
	typecheck_binary_expr,					/**< Invalid argument type to binary expression */
	expected_colon_in_conditional,			/**< Expected ':' in condtional expression */
	typecheck_cond_incompatible_operands,	/**< Incompatible operand types */
	typecheck_statement_requires_scalar,	/**< Condition must be of scalar type */

	// Statement errors
	expected_semi_after_stmt,			/**< Expected ';' after statement */
	case_not_in_switch,					/**< 'case' statement not in switch statement */
	float_in_switch,
	expected_colon_after_case,			/**< Expected ':' after 'case' */
	default_not_in_switch,				/**< 'default' statement not in switch statement */
	expected_colon_after_default,		/**< Expected ':' after 'default' */
	expected_while,						/**< Expected 'while' in do/while loop */
	no_leftbr_in_for,
	no_semicolon_in_for,
	no_rightbr_in_for,
	no_ident_after_goto,
	continue_not_in_loop,				/**< 'continue' statement not in loop statement */
	break_not_in_loop_or_switch,		/**< 'break' statement not in loop or switch statement */
	no_ret_in_func,
	bad_type_in_ret,
	notvoidret_in_void_func,

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
	wrong_number_of_params,
	wait_rightbr_in_primary,
	unassignable_inc,
	wrong_addr,
	no_colon_in_cond_expr,
	int_op_for_float,
	not_const_expr,
	not_const_int_expr,
	assmnt_float_to_int,
	redefinition_of_main,
	no_leftbr_in_printid,
	no_rightbr_in_printid,
	no_ident_in_printid,
	no_leftbr_in_getid,
	no_rightbr_in_getid,
	no_ident_in_getid,
	init_int_by_float,
	no_comma_in_setmotor,
	param_setmotor_not_int,
	no_leftbr_in_stand_func,
	no_rightbr_in_stand_func,
	bad_param_in_stand_func,
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
	must_be_digit_after_exp,
	label_not_declared,
	repeated_label,
	operand_is_pointer,
	pointer_in_print,
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

	wrong_arg_in_send = 341,
	wrong_arg_in_create,

	no_leftbr_in_printf,
	no_rightbr_in_printf,
	wrong_first_printf_param,
	wrong_printf_param_type,
	wrong_printf_param_number,
	printf_no_format_placeholder,
	printf_unknown_format_placeholder,
	too_many_printf_args,

	no_mult_in_cast,
	no_rightbr_in_cast,
	not_pointer_in_cast,
	empty_bound_without_init,
	begin_with_notarray,
	string_and_notstring,
	wrong_init_in_actparam,
	no_comma_or_end,

	not_string_in_stanfunc = 362,
	not_int_in_stanfunc,
	no_comma_in_act_params_stanfunc,
	not_point_string_in_stanfunc,

	struct_init_must_start_from_BEGIN,
	not_rowofint_in_stanfunc,
	not_rowoffloat_in_stanfunc,

	not_float_in_stanfunc,
	not_array_in_stanfunc,

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
	array_borders_cannot_be_static_dynamic,
	such_array_is_not_supported,
	too_many_arguments
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
