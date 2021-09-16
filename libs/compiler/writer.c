/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "writer.h"
#include "uniprinter.h"


#define INDENT			"  "


/** AST writer */
typedef struct writer
{
	const syntax *sx;					/**< Syntax structure */
	universal_io *io;					/**< Output file */
} writer;

static void write_expression(writer *const wrt, const node *const nd, const size_t indent);
static void write_statement(writer *const wrt, const node *const nd, const size_t indent);


//===----------------------------------------------------------------------===//
//                                Writer Utils                                //
//===----------------------------------------------------------------------===//

static void write(writer *const wrt, const char *const string)
{
	uni_printf(wrt->io, "%s", string);
}

/**
 *	Write indentation
 *
 *	@param	wrt			Writer
 *	@param	indent		Indentation
 */
static void write_indent(writer *const wrt, const size_t indent)
{
	for (size_t i = 0; i < indent; i++)
	{
		write(wrt, INDENT);
	}
}

/**
 *	Write source location
 *
 *	@param	wrt			Writer
 *	@param	loc			Source location
 */
static void write_location(writer *const wrt, const location loc)
{
	uni_printf(wrt->io, " at <%lu, %lu>\n", loc.begin, loc.end);
}

/**
 *	Write type spelling
 *
 *	@param	wrt			Writer
 *	@param	type		Type
 */
static void write_type(writer *const wrt, const item_t type)
{
	if (type_is_null_pointer(type))
	{
		write(wrt, "nullptr");
	}
	else if (type_is_integer(type))
	{
		write(wrt, "int");
	}
	else if (type_is_floating(type))
	{
		write(wrt, "float");
	}
	else if (type_is_file(type))
	{
		write(wrt, "FILE");
	}
	else if (type_is_void(type))
	{
		write(wrt, "void");
	}
	else if (type_is_array(wrt->sx, type))
	{
		write_type(wrt, type_array_get_element_type(wrt->sx, type));
		write(wrt, "[]");
	}
	else if (type_is_pointer(wrt->sx, type))
	{
		write_type(wrt, type_pointer_get_element_type(wrt->sx, type));
		write(wrt, "*");
	}
	else if (type_is_structure(wrt->sx, type))
	{
		write(wrt, "struct { ");

		const size_t member_amount = type_structure_get_member_amount(wrt->sx, type);
		for (size_t i = 0; i < member_amount; i++)
		{
			const item_t member_type = type_structure_get_member_type(wrt->sx, type, i);
			const size_t member_repr = type_structure_get_member_name(wrt->sx, type, i);

			write_type(wrt, member_type);
			uni_printf(wrt->io, " %s; ", repr_get_name(wrt->sx, member_repr));
		}

		write(wrt, "}");
	}
	else if (type_is_function(wrt->sx, type))
	{
		write_type(wrt, type_function_get_return_type(wrt->sx, type));
		write(wrt, " (");

		const size_t parameter_amount = type_function_get_parameter_amount(wrt->sx, type);
		for (size_t i = 0; i < parameter_amount; i++)
		{
			const item_t parameter_type = type_function_get_parameter_type(wrt->sx, type, i);

			write_type(wrt, parameter_type);
			if (i != parameter_amount - 1)
			{
				write(wrt, ", ");
			}
		}
		write_type(wrt, type_pointer_get_element_type(wrt->sx, type));
		write(wrt, ")");
	}
}

/**
 *	Write unary operator spelling
 *
 *	@param	wrt			Writer
 *	@param	operator	Operator
 */
static void write_unary_operator(writer *const wrt, const unary_t operator)
{
	switch (operator)
	{
		case UN_POSTINC:
			write(wrt, "'postfix ++'");
			break;
		case UN_POSTDEC:
			write(wrt, "'postfix --'");
			break;
		case UN_PREINC:
			write(wrt, "'++'");
			break;
		case UN_PREDEC:
			write(wrt, "'--'");
			break;
		case UN_ADDRESS:
			write(wrt, "'&'");
			break;
		case UN_INDIRECTION:
			write(wrt, "'*'");
			break;
		case UN_PLUS:
			write(wrt, "'+'");
			break;
		case UN_MINUS:
			write(wrt, "'-'");
			break;
		case UN_NOT:
			write(wrt, "'~'");
			break;
		case UN_LOGNOT:
			write(wrt, "'!'");
			break;
		case UN_ABS:
			write(wrt, "'abs'");
			break;
	}
}

/**
 *	Write binary operator spelling
 *
 *	@param	wrt			Writer
 *	@param	operator	Operator
 */
static void write_binary_operator(writer *const wrt, const binary_t operator)
{
	switch (operator)
	{
		case BIN_MUL:
			write(wrt, "'*'");
			break;
		case BIN_DIV:
			write(wrt, "'/'");
			break;
		case BIN_REM:
			write(wrt, "'%'");
			break;
		case BIN_ADD:
			write(wrt, "'+'");
			break;
		case BIN_SUB:
			write(wrt, "'-'");
			break;
		case BIN_SHL:
			write(wrt, "'<<'");
			break;
		case BIN_SHR:
			write(wrt, "'>>'");
			break;
		case BIN_LT:
			write(wrt, "'<'");
			break;
		case BIN_GT:
			write(wrt, "'>'");
			break;
		case BIN_LE:
			write(wrt, "'<='");
			break;
		case BIN_GE:
			write(wrt, "'>='");
			break;
		case BIN_EQ:
			write(wrt, "'=='");
			break;
		case BIN_NE:
			write(wrt, "'!='");
			break;
		case BIN_AND:
			write(wrt, "'&'");
			break;
		case BIN_XOR:
			write(wrt, "'^'");
			break;
		case BIN_OR:
			write(wrt, "'|'");
			break;
		case BIN_LOG_AND:
			write(wrt, "'&&'");
			break;
		case BIN_LOG_OR:
			write(wrt, "'||'");
			break;
		case BIN_ASSIGN:
			write(wrt, "'='");
			break;
		case BIN_MUL_ASSIGN:
			write(wrt, "'*='");
			break;
		case BIN_DIV_ASSIGN:
			write(wrt, "'/='");
			break;
		case BIN_REM_ASSIGN:
			write(wrt, "'%='");
			break;
		case BIN_ADD_ASSIGN:
			write(wrt, "'+='");
			break;
		case BIN_SUB_ASSIGN:
			write(wrt, "'-='");
			break;
		case BIN_SHL_ASSIGN:
			write(wrt, "'<<='");
			break;
		case BIN_SHR_ASSIGN:
			write(wrt, "'>>='");
			break;
		case BIN_AND_ASSIGN:
			write(wrt, "'&='");
			break;
		case BIN_XOR_ASSIGN:
			write(wrt, "'^='");
			break;
		case BIN_OR_ASSIGN:
			write(wrt, "'|='");
			break;
		case BIN_COMMA:
			write(wrt, "','");
			break;
	}
}


//===----------------------------------------------------------------------===//
//                            Expression Writing                              //
//===----------------------------------------------------------------------===//

/**
 *	Write expression metadata
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_expression_metadata(writer *const wrt, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
	   write(wrt, " ─ lvalue");
	}
	else
	{
		write(wrt, " ─ rvalue");
	}

	write(wrt, " of type '");
	write_type(wrt, expression_get_type(nd));
	write(wrt, "'");

	write_location(wrt, expression_get_location(nd));
}

/**
 *	Write identifier expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_identifier_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_IDENTIFIER");

	const size_t id = expression_identifier_get_id(nd);
	const char *spelling = ident_get_spelling(wrt->sx, id);
	uni_printf(wrt->io, " named \'%s\' with id %zu", spelling, id);

	write_expression_metadata(wrt, nd);
}

/**
 *	Write literal expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_literal_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_LITERAL");

	const item_t type = expression_get_type(nd);
	if (type_is_integer(type))
	{
		const int value = expression_literal_get_integer(nd);
		uni_printf(wrt->io, " with value %i", value);
	}
	else if (type_is_floating(type))
	{
		const double value = expression_literal_get_floating(nd);
		uni_printf(wrt->io, " with value %f", value);
	}
	else if (type_is_string(wrt->sx, type))
	{
		const size_t string_num = expression_literal_get_string(nd);
		const char *const string = string_get(wrt->sx, string_num);
		uni_printf(wrt->io, " with value \"%s\"", string);
	}
	// Nothing to write for null pointer constant

	write_expression_metadata(wrt, nd);
}

/**
 *	Write subscript expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_subscript_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_SUBSCRIPT");
	write_expression_metadata(wrt, nd);

	const node base = expression_subscript_get_base(nd);
	write_expression(wrt, &base, indent + 1);

	const node index = expression_subscript_get_index(nd);
	write_expression(wrt, &index, indent + 1);
}

/**
 *	Write call expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_call_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_CALL");
	write_expression_metadata(wrt, nd);

	const node callee = expression_call_get_callee(nd);
	write_expression(wrt, &callee, indent + 1);

	const size_t arguments_amount = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < arguments_amount; i++)
	{
		const node argument = expression_call_get_argument(nd, i);
		write_expression(wrt, &argument, indent + 1);
	}
}

/**
 *	Write member expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_member_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_MEMBER selecting");

	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);
	const size_t index = expression_member_get_member_index(nd);

	if (expression_member_is_arrow(nd))
	{
		write(wrt, " ->");

		const item_t structure_type = type_pointer_get_element_type(wrt->sx, base_type);
		const size_t repr = type_structure_get_member_name(wrt->sx, structure_type, index);
		write(wrt, repr_get_name(wrt->sx, repr));
	}
	else
	{
		write(wrt, " .");

		const size_t repr = type_structure_get_member_name(wrt->sx, base_type, index);
		write(wrt, repr_get_name(wrt->sx, repr));
	}

	uni_printf(wrt->io, " by index #%zu", index);
	write_expression_metadata(wrt, nd);
	write_expression(wrt, &base, indent + 1);
}

/**
 *	Write unary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_unary_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_UNARY with operator ");
	write_unary_operator(wrt, expression_unary_get_operator(nd));
	write_expression_metadata(wrt, nd);

	const node operand = expression_unary_get_operand(nd);
	write_expression(wrt, &operand, indent + 1);
}

/**
 *	Write binary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_binary_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_BINARY with operator ");
	write_binary_operator(wrt, expression_binary_get_operator(nd));
	write_expression_metadata(wrt, nd);

	const node LHS = expression_binary_get_LHS(nd);
	write_expression(wrt, &LHS, indent + 1);

	const node RHS = expression_binary_get_RHS(nd);
	write_expression(wrt, &RHS, indent + 1);
}

/**
 *	Write ternary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_ternary_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_TERNARY");
	write_expression_metadata(wrt, nd);

	const node condition = expression_ternary_get_condition(nd);
	write_expression(wrt, &condition, indent + 1);

	const node LHS = expression_binary_get_LHS(nd);
	write_expression(wrt, &LHS, indent + 1);

	const node RHS = expression_binary_get_RHS(nd);
	write_expression(wrt, &RHS, indent + 1);
}

/**
 *	Write expression list
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_expression_list(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "EXPR_LIST");
	write_expression_metadata(wrt, nd);

	const size_t size = expression_list_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node subexpr = expression_list_get_subexpr(nd, i);
		write_expression(wrt, &subexpr, indent + 1);
	}
}

/**
 *	Write expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_expression(writer *const wrt, const node *const nd, const size_t indent)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			write_identifier_expression(wrt, nd, indent);
			return;

		case EXPR_LITERAL:
			write_literal_expression(wrt, nd, indent);
			return;

		case EXPR_SUBSCRIPT:
			write_subscript_expression(wrt, nd, indent);
			return;

		case EXPR_CALL:
			write_call_expression(wrt, nd, indent);
			return;

		case EXPR_MEMBER:
			write_member_expression(wrt, nd, indent);
			return;

		case EXPR_UNARY:
			write_unary_expression(wrt, nd, indent);
			return;

		case EXPR_BINARY:
			write_binary_expression(wrt, nd, indent);
			return;

		case EXPR_TERNARY:
			write_ternary_expression(wrt, nd, indent);
			return;

		case EXPR_LIST:
			write_expression_list(wrt, nd, indent);
			return;
	}
}


//===----------------------------------------------------------------------===//
//                            Declaration Writing                             //
//===----------------------------------------------------------------------===//

/**
 *	Write variable declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_variable_declaration(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "DECL_VAR");

	const size_t ident = declaration_variable_get_id(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, ident);
	const item_t type = ident_get_type(wrt->sx, ident);

	uni_printf(wrt->io, " declaring variable named \'%s\' with id %zu of type '", spelling, ident);
	write_type(wrt, type);
	write(wrt, "'\n");

	if (declaration_variable_has_initializer(nd))
	{
		const node initializer = declaration_variable_get_initializer(nd);
		write_expression(wrt, &initializer, indent + 1);
	}
}

/**
 *	Write type declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_type_declaration(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "DECL_TYPE\n");
	(void)nd;
}

/**
 *	Write function declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_function_declaration(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "DECL_FUNC\n");

	const size_t ident = declaration_function_get_id(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, ident);
	const item_t type = ident_get_type(wrt->sx, ident);

	uni_printf(wrt->io, " declaring function named \'%s\' with id %zu of type '", spelling, ident);
	write_type(wrt, type);
	write(wrt, "'\n");

	const node body = declaration_function_get_body(nd);
	write_statement(wrt, &body, indent + 1);
}

/**
 *	Write declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_declaration(writer *const wrt, const node *const nd, const size_t indent)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			write_variable_declaration(wrt, nd, indent);
			return;

		case DECL_TYPE:
			write_type_declaration(wrt, nd, indent);
			return;

		case DECL_FUNC:
			write_function_declaration(wrt, nd, indent);
			return;
	}
}


//===----------------------------------------------------------------------===//
//                             Statement Writing                              //
//===----------------------------------------------------------------------===//

/**
 *	Write labeled statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_labeled_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_LABEL");

	const size_t label = statement_labeled_get_label(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, label);
	uni_printf(wrt->io, " declaring label named \'%s\' with id %zu\n", spelling, label);

	const node substmt = statement_labeled_get_substmt(nd);
	write_statement(wrt, &substmt, indent + 1);
}

/**
 *	Write case statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_case_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_CASE\n");

	const node expression = statement_case_get_expression(nd);
	write_expression(wrt, &expression, indent + 1);

	const node substmt = statement_case_get_substmt(nd);
	write_statement(wrt, &substmt, indent + 1);
}

/**
 *	Write default statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_default_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_DEFAULT\n");

	const node substmt = statement_default_get_substmt(nd);
	write_statement(wrt, &substmt, indent + 1);
}

/**
 *	Write compound statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_compound_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_COMPOUND\n");

	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		write_statement(wrt, &substmt, indent + 1);
	}
}

/**
 *	Write null statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_null_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_NULL\n");
	(void)nd;
}

/**
 *	Write if statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_if_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_IF\n");

	const node condition = statement_if_get_condition(nd);
	write_expression(wrt, &condition, indent + 1);

	const node then_substmt = statement_if_get_then_substmt(nd);
	write_statement(wrt, &then_substmt, indent + 1);

	if (statement_if_has_else_substmt(nd))
	{
		const node else_substmt = statement_if_get_else_substmt(nd);
		write_statement(wrt, &else_substmt, indent + 1);
	}
}

/**
 *	Write switch statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_switch_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_SWITCH\n");

	const node condition = statement_switch_get_condition(nd);
	write_expression(wrt, &condition, indent + 1);

	const node body = statement_switch_get_body(nd);
	write_statement(wrt, &body, indent + 1);
}

/**
 *	Write while statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_while_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_WHILE\n");

	const node condition = statement_while_get_condition(nd);
	write_expression(wrt, &condition, indent + 1);

	const node body = statement_while_get_body(nd);
	write_statement(wrt, &body, indent + 1);
}

/**
 *	Write do statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_do_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_DO\n");

	const node body = statement_while_get_body(nd);
	write_statement(wrt, &body, indent + 1);

	const node condition = statement_while_get_condition(nd);
	write_expression(wrt, &condition, indent + 1);
}

/**
 *	Write for statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_for_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_FOR\n");

	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		write_statement(wrt, &inition, indent + 1);
	}

	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		write_expression(wrt, &condition, indent + 1);
	}

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		write_statement(wrt, &increment, indent + 1);
	}

	const node body = statement_for_get_body(nd);
	write_statement(wrt, &body, indent + 1);
}

/**
 *	Write goto statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_goto_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_GOTO\n");

	const size_t label = statement_goto_get_label(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, label);
	uni_printf(wrt->io, " label named \'%s\' with id %zu\n", spelling, label);
}

/**
 *	Write continue statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_continue_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_CONTINUE\n");

	(void)nd;
}

/**
 *	Write break statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_break_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_BREAK\n");

	(void)nd;
}

/**
 *	Write return statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_return_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_RETURN\n");

	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		write_expression(wrt, &expression, indent + 1);
	}
}

/**
 *	Write printf statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_printf_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_PRINTF\n");

	const node format_str = statement_printf_get_format_str(nd);
	write_expression(wrt, &format_str, indent + 1);

	const size_t argc = statement_printf_get_argc(nd);
	for (size_t i = 0; i < argc; i++)
	{
		const node argument = statement_printf_get_argument(nd, i);
		write_expression(wrt, &argument, indent + 1);
	}
}

/**
 *	Write print statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_print_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_PRINT\n");

	const node argument = node_get_child(nd, 0);
	write_expression(wrt, &argument, indent + 1);
}

/**
 *	Write printid statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_printid_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_PRINTID");

	const size_t id = (size_t)node_get_arg(nd, 0);
	uni_printf(wrt->io, " id=%zu\n", id);
}

/**
 *	Write getid statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_getid_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	write_indent(wrt, indent);
	write(wrt, "STMT_GETID");

	const size_t id = (size_t)node_get_arg(nd, 0);
	uni_printf(wrt->io, " id=%zu\n", id);
}

/**
 *	Write statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 *	@param	indent		Indentation
 */
static void write_statement(writer *const wrt, const node *const nd, const size_t indent)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			write_declaration(wrt, nd, indent);
			return;

		case STMT_LABEL:
			write_labeled_statement(wrt, nd, indent);
			return;

		case STMT_CASE:
			write_case_statement(wrt, nd, indent);
			return;

		case STMT_DEFAULT:
			write_default_statement(wrt, nd, indent);
			return;

		case STMT_COMPOUND:
			write_compound_statement(wrt, nd, indent);
			return;

		case STMT_EXPR:
			write_expression(wrt, nd, indent);
			return;

		case STMT_NULL:
			write_null_statement(wrt, nd, indent);
			return;

		case STMT_IF:
			write_if_statement(wrt, nd, indent);
			return;

		case STMT_SWITCH:
			write_switch_statement(wrt, nd, indent);
			return;

		case STMT_WHILE:
			write_while_statement(wrt, nd, indent);
			return;

		case STMT_DO:
			write_do_statement(wrt, nd, indent);
			return;

		case STMT_FOR:
			write_for_statement(wrt, nd, indent);
			return;

		case STMT_GOTO:
			write_goto_statement(wrt, nd, indent);
			return;

		case STMT_CONTINUE:
			write_continue_statement(wrt, nd, indent);
			return;

		case STMT_BREAK:
			write_break_statement(wrt, nd, indent);
			return;

		case STMT_RETURN:
			write_return_statement(wrt, nd, indent);
			return;

		case STMT_PRINTF:
			write_printf_statement(wrt, nd, indent);
			break;
		case STMT_PRINT:
			write_print_statement(wrt, nd, indent);
			break;
		case STMT_PRINTID:
			write_printid_statement(wrt, nd, indent);
			break;
		case STMT_GETID:
			write_getid_statement(wrt, nd, indent);
			break;
	}
}

/**
 *	Write translation unit
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_translation_unit(writer *const wrt, const node *const nd)
{
	write(wrt, "Translation unit\n");

	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node declaration = translation_unit_get_declaration(nd, i);
		write_declaration(wrt, &declaration, 1);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void write_AST(const char *const path, syntax *const sx)
{
	universal_io io = io_create();
	if (path == NULL || sx == NULL || out_set_file(&io, path))
	{
		return;
	}

	writer wrt = { .sx = sx, .io = &io };

	const node root = node_get_root(&sx->tree);
	write_translation_unit(&wrt, &root);
	//wrt_clear(&wrt);

	io_erase(&io);
}
