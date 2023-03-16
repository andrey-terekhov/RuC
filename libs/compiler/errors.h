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
	extraneous_bracket_before_semi,			/**< Extraneous bracket before ';' */
	expected_r_paren,						/**< Expected ')' */
	expected_r_square,						/**< Expected ']' */
	expected_r_brace,						/**< Expected '}' */
	expected_expression,					/**< Expected expression */
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
	expected_l_paren_after_for,				/**< Expected '(' after 'for' */
	expected_semi_in_for_specifier,			/**< Expected ';' in 'for' statement specifier */
	continue_not_in_loop,					/**< 'continue' statement not in loop statement */
	break_not_in_loop_or_switch,			/**< 'break' statement not in loop or switch statement */

	// Semantics errors
	use_of_undeclared_identifier,			/**< Use of undeclared identifier */
	subscripted_expr_not_array,				/**< Subscripted expression is not an array */
	array_subscript_not_integer,			/**< Array subscript is not an integer */
	called_expr_not_function,				/**< Called expression is not a function */
	wrong_argument_amount,					/**< Wrong argument amount in call expression */
	member_reference_not_struct,			/**< Member reference base type is not a structure */
	member_reference_not_struct_pointer,	/**< Member reference type is not a structure pointer */
	no_such_member,							/**< No such member */
	unassignable_expression,				/**< Expression is not assignable */
	increment_operand_not_arithmetic,		/**< Operand of increment/decrement must be of arithmetic type */
	addrof_operand_not_lvalue,				/**< Cannot take the address of an rvalue */
	indirection_operand_not_pointer,		/**< Indirection operand is not a pointer */
	unary_operand_not_arithmetic,			/**< Operand of this unary operator must be of arithemtic type */
	unnot_operand_not_integer,				/**< Operand of '~' is not an integer */
	lognot_operand_not_scalar,				/**< Operand of '!' must be of scalar type */
	upb_operand_not_array,					/**< Operand of 'upb' is not an array */
	typecheck_binary_expr,					/**< Invalid argument types to binary expression */
	condition_must_be_scalar,				/**< Condition must be of scalar type */
	expected_constant_expression,			/**< Expected constant expression */
	incompatible_cond_operands,				/**< Incompatible operand types in conditional expression */
	expected_member_name,					/**< Expected member name */
	array_member_must_have_bounds,			/**< Array members must have size expressions */
	expected_identifier_in_declarator,		/**< Expected identifier in declarator */
	declaration_does_not_declare_anything,	/**< Declaration does not declare anything */
	case_expr_not_integer,					/**< Case expression is not an integer */
	switch_expr_not_integer,				/**< Switch expression is not an integer */
	void_func_valued_return,				/**< Void function should not return a value */
	nonvoid_func_void_return,				/**< Non-void function should return a value */
	bad_type_in_ret,
	wrong_init,
	main_should_return_int_or_void,
	main_should_be_defined,
	wrong_main_parameters,
	wrong_main_parameter_type,
	assign_to_const,
	invalid_const_pointer_cast,

	// Builtin errors
	too_many_printf_args,					/**< Too many printf arguments */
	expected_format_specifier,				/**< Expected format specifier */
	unknown_format_specifier,				/**< Unknown format specifier */
	printf_fst_not_string,					/**< First argument of 'printf' call is not a string literal */
	wrong_printf_argument_amount,			/**< Wrong argument amount in 'printf' call */
	wrong_printf_argument_type,				/**< Wrong argument type in 'printf' call */
	pointer_in_print,						/**< Pointer in print */
	expected_identifier_in_printid,			/**< Expected identifier in 'printid' call */
	expected_identifier_in_getid,			/**< Expected identifier in 'getid' call */

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
	repeated_decl,
	ident_is_not_declared,
	not_const_int_expr,
	redefinition_of_main,
	aster_before_func,
	aster_with_row,
	wrong_func_as_arg,
	no_right_br_in_arg_func,
	par_type_void_with_nofun,
	ident_in_declarator,
	array_before_func,
	wait_definition,
	wait_declarator,
	two_idents_for_1_declarer,
	function_has_no_body,
	wrong_struct,
	pnt_before_array,
	array_size_must_be_int,
	no_semicolon_in_struct,
	no_comma_in_enum,
	wait_ident_after_semicolon_in_struct,
	wait_ident_after_comma_in_enum,
	empty_init,
	ident_not_type,
	not_decl,
	multiple_const_in_type,

	empty_bound_without_init,

	// Tree testing errors
	node_unexpected,

	// Codegen errors
	tables_cannot_be_compressed,
	wrong_init_in_actparam,
	array_borders_cannot_be_static_dynamic,
	such_array_is_not_supported,
	too_many_arguments
} err_t;

/** Warnings codes */
typedef enum WARNING
{
	result_of_assignment_as_condition,		/**< Using the result of an assignment as a condition */
	too_long_int,
	variable_deviation,
} warning_t;


/**
 *	Emit an error for some problem
 *
 *	@param	io			Universal io
 *	@param	num			Error number
 */
void error(const universal_io *const io, err_t num, ...);

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
void verror(const universal_io *const io, const err_t num, va_list args);

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
void system_error(err_t num, ...);

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
