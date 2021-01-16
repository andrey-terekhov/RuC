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
enum ERROR
{
	// Lexer errors
	bad_character,						/**< "Bad character in source" error */
	empty_character,					/**< "Empty character constant" error */
	unknown_escape_sequence,			/**< "Unknown escape sequence" error */
	expected_apost_after_char_const,	/**< "Missing terminating ' character" error */
	missing_terminating_quote_char,		/**< "Missing terminating '"' character" error */
	string_too_long,					/**< "String literal exceeds maximum length" error */
	unterminated_block_comment,			/**< "Unterminated block comment" error */
	
	// Environment errors
	no_main_in_program,					/**< "Undefined main" error */
	predef_but_notdef,					/**< "Undefined function" error */

	// Other errors
	after_type_must_be_ident = 201,
	wait_right_sq_br,
	only_functions_may_have_type_VOID,
	decl_and_def_have_diff_type,
	wrong_param_list,
	def_must_end_with_semicomma,
	func_decl_req_params,
	wait_while_in_do_stmt,
	no_semicolon_after_stmt,
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
	no_colon_in_case,
	case_after_default,
	no_ident_after_goto,
	no_leftbr_in_for,
	no_semicolon_in_for,
	no_rightbr_in_for,
	int_op_for_float,
	assmnt_float_to_int,
	redefinition_of_main,
	no_leftbr_in_printid,
	no_rightbr_in_printid,
	no_ident_in_printid,
	float_in_switch,
	init_int_by_float,
	no_comma_in_setmotor,
	param_setmotor_not_int,
	no_leftbr_in_stand_func,
	no_rightbr_in_stand_func,
	bad_param_in_stand_func,
	no_ret_in_func,
	bad_type_in_ret,
	notvoidret_in_void_func,
	decl_after_strmt,
	aster_before_func,
	aster_not_for_pointer,
	aster_with_row,
	float_in_condition,
	wrong_fun_as_param,
	no_right_br_in_paramfun,
	par_type_void_with_nofun,
	ident_in_declarator,
	array_before_func,
	wait_definition,
	wait_declarator,
	two_idents_for_1_declarer,
	function_has_no_body,
	diff_formal_param_type_and_actual,
	case_or_default_not_in_switch,
	break_not_in_loop_or_switch,
	continue_not_in_loop,
	not_primary,
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
	type_missmatch,
	array_assigment,
	wrong_struct_ass,
	wrong_init,
	no_field,
	slice_from_func,
	bad_toval,
	wait_end,
	act_param_not_ident,
	unassignable,
	pnt_before_array,
	array_size_must_be_int,
	no_semicomma_in_struct,
	wait_ident_after_semicomma_in_struct,
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
	too_many_printf_params,

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

	tree_expression_not_block,
	tree_expression_texprend,
	tree_expression_unknown,
	tree_expression_operator,
	tree_expression_no_texprend,
	tree_no_tend,
	tree_unexpected,
};

/** Warnings codes */
enum WARNING
{
	too_long_int = 400,

	tree_operator_unknown,
	node_argc,
};


/**
 *	Emit an error for some problem
 *
 *	@param	io		Universal io
 *	@param	num		Error number
 */
void error(const universal_io *const io, const int num, ...);

/**
 *	Emit a warning for some problem
 *
 *	@param	io		Universal io
 *	@param	num		Warning number
 */
void warning(const universal_io *const io, const int num, ...);


/**
 *	Emit error message
 *
 *	@param	io		Universal io
 *	@param	msg		Error message
 */
void error_msg(const universal_io *const io, const char *const msg);

/**
 *	Emit warning message
 *
 *	@param	io		Universal io
 *	@param	msg		Warning message
 */
void warning_msg(const universal_io *const io, const char *const msg);


/**
 *	Emit error by number
 *
 *	@param	msg		Error message
 */
void system_error(const char *const msg);

/**
 *	Emit warning by number
 *
 *	@param	msg		Warning message
 */
void system_warning(const char *const msg);

#ifdef __cplusplus
} /* extern "C" */
#endif
