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
#include <string.h>
#include "AST.h"
#include "instructions.h"
#include "uniprinter.h"


#define MAX_ELEM_SIZE	32
#define INDENT			"  "


/** AST writer */
typedef struct writer
{
	const syntax *sx;					/**< Syntax structure */
	universal_io *io;					/**< Output file */
	size_t indent;						/**< Indentation count */
} writer;


static void write_expression(writer *const wrt, const node *const nd);
static void write_statement(writer *const wrt, const node *const nd);
static void write_declaration(writer *const wrt, const node *const nd);


/*
 *	 __  __     ______   __     __         ______
 *	/\ \/\ \   /\__  _\ /\ \   /\ \       /\  ___\
 *	\ \ \_\ \  \/_/\ \/ \ \ \  \ \ \____  \ \___  \
 *	 \ \_____\    \ \_\  \ \_\  \ \_____\  \/\_____\
 *	  \/_____/     \/_/   \/_/   \/_____/   \/_____/
 */


/**
 *	Write string
 *
 *	@param	wrt			Writer
 *	@param	string		String
 */
static inline void write(writer *const wrt, const char *const string)
{
	uni_printf(wrt->io, "%s", string);
}

/**
 *	Write line
 *
 *	@param	wrt			Writer
 *	@param	string		String
 */
static inline void write_line(writer *const wrt, const char *const string)
{
	for (size_t i = 0; i < wrt->indent; i++)
	{
		write(wrt, INDENT);
	}

	uni_printf(wrt->io, "%s", string);
}

/**
 *	Write source location
 *
 *	@param	wrt			Writer
 *	@param	loc			Source location
 */
static inline void write_location(writer *const wrt, const location loc)
{
	uni_printf(wrt->io, " at <%zu, %zu>\n", loc.begin, loc.end);
}

/**
 *	Write type spelling
 *
 *	@param	wrt			Writer
 *	@param	type		Type
 */
static void write_type(writer *const wrt, const item_t type)
{
	char spelling[MAX_STRING_LENGTH];
	write_type_spelling(wrt->sx, type, spelling);
	write(wrt, spelling);
}

/**
 *	Write unary operator spelling
 *
 *	@param	wrt			Writer
 *	@param	op			Operator
 */
static void write_unary_operator(writer *const wrt, const unary_t op)
{
	switch (op)
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
		case UN_UPB:
			write(wrt, "'upb'");
			break;
	}
}

/**
 *	Write binary operator spelling
 *
 *	@param	wrt			Writer
 *	@param	op			Operator
 */
static void write_binary_operator(writer *const wrt, const binary_t op)
{
	switch (op)
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


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


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

	write_location(wrt, node_get_location(nd));
}

/**
 *	Write identifier expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_identifier_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_IDENTIFIER");

	const size_t id = expression_identifier_get_id(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, id);
	uni_printf(wrt->io, " named \'%s\' with id %zu", spelling, id);

	write_expression_metadata(wrt, nd);
}

/**
 *	Write literal expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_literal_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_LITERAL with value ");

	const item_t type = expression_get_type(nd);
	switch (type_get_class(wrt->sx, type))
	{
		case TYPE_NULL_POINTER:
			write(wrt, "NULL");
			break;

		case TYPE_BOOLEAN:
			uni_printf(wrt->io, expression_literal_get_boolean(nd) ? "true" : "false");
			break;

		case TYPE_CHARACTER:
			uni_printf(wrt->io, "%c", expression_literal_get_character(nd));
			break;

		case TYPE_INTEGER:
			uni_printf(wrt->io, "%" PRIitem, expression_literal_get_integer(nd));
			break;

		case TYPE_FLOATING:
			uni_printf(wrt->io, "%f", expression_literal_get_floating(nd));
			break;

		case TYPE_ARRAY:
		{
			const size_t string_num = expression_literal_get_string(nd);
			const char *const string = string_get(wrt->sx, string_num);
			uni_printf(wrt->io, "\"%s\"", string);
		}
		break;

		default:
			break;
	}

	write_expression_metadata(wrt, nd);
}

/**
 *	Write subscript expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_subscript_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_SUBSCRIPT");
	write_expression_metadata(wrt, nd);

	const node base = expression_subscript_get_base(nd);
	write_expression(wrt, &base);

	const node index = expression_subscript_get_index(nd);
	write_expression(wrt, &index);
}

/**
 *	Write call expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_call_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_CALL");
	write_expression_metadata(wrt, nd);

	const node callee = expression_call_get_callee(nd);
	write_expression(wrt, &callee);

	const size_t arguments_amount = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < arguments_amount; i++)
	{
		const node argument = expression_call_get_argument(nd, i);
		write_expression(wrt, &argument);
	}
}

/**
 *	Write member expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_member_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_MEMBER selecting");

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
	write_expression(wrt, &base);
}

/**
 *	Write cast expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_cast_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_CAST from ");

	const item_t target_type = expression_get_type(nd);
	const item_t source_type = expression_cast_get_source_type(nd);

	write_type(wrt, source_type);
	write(wrt, " to ");
	write_type(wrt, target_type);
	write_expression_metadata(wrt, nd);

	const node operand = expression_cast_get_operand(nd);
	write_expression(wrt, &operand);
}

/**
 *	Write unary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_unary_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_UNARY with operator ");
	write_unary_operator(wrt, expression_unary_get_operator(nd));
	write_expression_metadata(wrt, nd);

	const node operand = expression_unary_get_operand(nd);
	write_expression(wrt, &operand);
}

/**
 *	Write binary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_binary_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_BINARY with operator ");
	write_binary_operator(wrt, expression_binary_get_operator(nd));
	write_expression_metadata(wrt, nd);

	const node LHS = expression_binary_get_LHS(nd);
	write_expression(wrt, &LHS);

	const node RHS = expression_binary_get_RHS(nd);
	write_expression(wrt, &RHS);
}

/**
 *	Write ternary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_ternary_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_TERNARY");
	write_expression_metadata(wrt, nd);

	const node condition = expression_ternary_get_condition(nd);
	write_expression(wrt, &condition);

	const node LHS = expression_ternary_get_LHS(nd);
	write_expression(wrt, &LHS);

	const node RHS = expression_ternary_get_RHS(nd);
	write_expression(wrt, &RHS);
}

/**
 *	Write assignment expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_assignment_expression(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_ASSIGNMENT with operator ");
	write_binary_operator(wrt, expression_assignment_get_operator(nd));
	write_expression_metadata(wrt, nd);

	const node LHS = expression_assignment_get_LHS(nd);
	write_expression(wrt, &LHS);

	const node RHS = expression_assignment_get_RHS(nd);
	write_expression(wrt, &RHS);
}

/**
 *	Write initializer
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_initializer(writer *const wrt, const node *const nd)
{
	write_line(wrt, "EXPR_INITIALIZER");
	write_expression_metadata(wrt, nd);

	const size_t size = expression_initializer_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node subexpr = expression_initializer_get_subexpr(nd, i);
		write_expression(wrt, &subexpr);
	}
}

/**
 *	Write expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_expression(writer *const wrt, const node *const nd)
{
	wrt->indent++;
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			write_identifier_expression(wrt, nd);
			break;

		case EXPR_LITERAL:
			write_literal_expression(wrt, nd);
			break;

		case EXPR_SUBSCRIPT:
			write_subscript_expression(wrt, nd);
			break;

		case EXPR_CALL:
			write_call_expression(wrt, nd);
			break;

		case EXPR_MEMBER:
			write_member_expression(wrt, nd);
			break;

		case EXPR_CAST:
			write_cast_expression(wrt, nd);
			break;

		case EXPR_UNARY:
			write_unary_expression(wrt, nd);
			break;

		case EXPR_BINARY:
			write_binary_expression(wrt, nd);
			break;

		case EXPR_TERNARY:
			write_ternary_expression(wrt, nd);
			break;

		case EXPR_ASSIGNMENT:
			write_assignment_expression(wrt, nd);
			break;

		case EXPR_INITIALIZER:
			write_initializer(wrt, nd);
			break;

		case EXPR_EMPTY_BOUND:
			write_line(wrt, "EXPR_EMPTY_BOUND\n");
			break;

		case EXPR_INVALID:
			write_line(wrt, "EXPR_INVALID\n");
			break;
	}

	wrt->indent--;
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 *	Write member declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_member_declaration(writer *const wrt, const node *const nd)
{
	write_line(wrt, "DECL_MEMBER");

	const size_t name = declaration_member_get_name(nd);
	const item_t type = declaration_member_get_type(nd);

	uni_printf(wrt->io, " declaring member named \'%s\' of type '", repr_get_name(wrt->sx, name));
	write_type(wrt, type);
	write(wrt, "'\n");

	const size_t amount = declaration_member_get_bounds_amount(nd);
	for (size_t i = 0; i < amount; i++)
	{
		const node bound = declaration_member_get_bound(nd, i);
		write_expression(wrt, &bound);
	}
}

/**
 *	Write struct declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_struct_declaration(writer *const wrt, const node *const nd)
{
	write_line(wrt, "DECL_STRUCT\n");

	const size_t amount = declaration_struct_get_size(nd);
	for (size_t i = 0; i < amount; i++)
	{
		const node member = declaration_struct_get_member(nd, i);
		write_declaration(wrt, &member);
	}
}

/**
 *	Write variable declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_variable_declaration(writer *const wrt, const node *const nd)
{
	write_line(wrt, "DECL_VAR");

	const size_t ident = declaration_variable_get_id(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, ident);
	const item_t type = ident_get_type(wrt->sx, ident);

	uni_printf(wrt->io, " declaring variable named \'%s\' with id %zu of type '", spelling, ident);
	write_type(wrt, type);
	write(wrt, "'\n");

	const size_t amount = declaration_variable_get_bounds_amount(nd);
	for (size_t i = 0; i < amount; i++)
	{
		const node bound = declaration_variable_get_bound(nd, i);
		write_expression(wrt, &bound);
	}

	if (declaration_variable_has_initializer(nd))
	{
		const node initializer = declaration_variable_get_initializer(nd);
		write_expression(wrt, &initializer);
	}
}

/**
 *	Write function declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_function_declaration(writer *const wrt, const node *const nd)
{
	write_line(wrt, "DECL_FUNC");

	const size_t ident = declaration_function_get_id(nd);
	const char *const spelling = ident_get_spelling(wrt->sx, ident);
	const item_t type = ident_get_type(wrt->sx, ident);

	uni_printf(wrt->io, " declaring function named \'%s\' with id %zu of type '", spelling, ident);
	write_type(wrt, type);
	write(wrt, "'\n");

	const node body = declaration_function_get_body(nd);
	write_statement(wrt, &body);
}

/**
 *	Write declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_declaration(writer *const wrt, const node *const nd)
{
	if (!node_is_correct(nd))
	{
		return;
	}

	wrt->indent++;
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			write_variable_declaration(wrt, nd);
			break;

		case DECL_MEMBER:
			write_member_declaration(wrt, nd);
			break;

		case DECL_STRUCT:
			write_struct_declaration(wrt, nd);
			break;

		case DECL_FUNC:
			write_function_declaration(wrt, nd);
			break;

		case DECL_INVALID:
			write(wrt, "DECL_INVALID\n");
			break;
	}

	wrt->indent--;
}


/*
 *	 ______     ______   ______     ______   ______     __    __     ______     __   __     ______   ______
 *	/\  ___\   /\__  _\ /\  __ \   /\__  _\ /\  ___\   /\ "-./  \   /\  ___\   /\ "-.\ \   /\__  _\ /\  ___\
 *	\ \___  \  \/_/\ \/ \ \  __ \  \/_/\ \/ \ \  __\   \ \ \-./\ \  \ \  __\   \ \ \-.  \  \/_/\ \/ \ \___  \
 *	 \/\_____\    \ \_\  \ \_\ \_\    \ \_\  \ \_____\  \ \_\ \ \_\  \ \_____\  \ \_\\"\_\    \ \_\  \/\_____\
 *	  \/_____/     \/_/   \/_/\/_/     \/_/   \/_____/   \/_/  \/_/   \/_____/   \/_/ \/_/     \/_/   \/_____/
 */


/**
 *	Write declaration statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_declaration_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_DECL\n");

	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		write_declaration(wrt, &decl);
	}
}

/**
 *	Write case statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_case_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_CASE\n");

	const node expression = statement_case_get_expression(nd);
	write_expression(wrt, &expression);

	const node substmt = statement_case_get_substmt(nd);
	write_statement(wrt, &substmt);
}

/**
 *	Write default statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_default_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_DEFAULT\n");

	const node substmt = statement_default_get_substmt(nd);
	write_statement(wrt, &substmt);
}

/**
 *	Write compound statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_compound_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_COMPOUND\n");

	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		write_statement(wrt, &substmt);
	}
}

/**
 *	Write null statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_null_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_NULL\n");
	(void)nd;
}

/**
 *	Write if statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_if_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_IF\n");

	const node condition = statement_if_get_condition(nd);
	write_expression(wrt, &condition);

	const node then_substmt = statement_if_get_then_substmt(nd);
	write_statement(wrt, &then_substmt);

	if (statement_if_has_else_substmt(nd))
	{
		const node else_substmt = statement_if_get_else_substmt(nd);
		write_statement(wrt, &else_substmt);
	}
}

/**
 *	Write switch statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_switch_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_SWITCH\n");

	const node condition = statement_switch_get_condition(nd);
	write_expression(wrt, &condition);

	const node body = statement_switch_get_body(nd);
	write_statement(wrt, &body);
}

/**
 *	Write while statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_while_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_WHILE\n");

	const node condition = statement_while_get_condition(nd);
	write_expression(wrt, &condition);

	const node body = statement_while_get_body(nd);
	write_statement(wrt, &body);
}

/**
 *	Write do statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_do_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_DO\n");

	const node body = statement_do_get_body(nd);
	write_statement(wrt, &body);

	const node condition = statement_do_get_condition(nd);
	write_expression(wrt, &condition);
}

/**
 *	Write for statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_for_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_FOR\n");

	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		write_statement(wrt, &inition);
	}

	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		write_expression(wrt, &condition);
	}

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		write_statement(wrt, &increment);
	}

	const node body = statement_for_get_body(nd);
	write_statement(wrt, &body);
}

/**
 *	Write continue statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_continue_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_CONTINUE\n");
	(void)nd;
}

/**
 *	Write break statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_break_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_BREAK\n");
	(void)nd;
}

/**
 *	Write return statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_return_statement(writer *const wrt, const node *const nd)
{
	write_line(wrt, "STMT_RETURN\n");

	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		write_expression(wrt, &expression);
	}
}

/**
 *	Write statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_statement(writer *const wrt, const node *const nd)
{
	if (!node_is_correct(nd))
	{
		return;
	}

	const statement_t class = statement_get_class(nd);
	if (class == STMT_EXPR)
	{
		write_expression(wrt, nd);
		return;
	}

	wrt->indent++;
	switch (class)
	{
		case STMT_DECL:
			write_declaration_statement(wrt, nd);
			break;

		case STMT_CASE:
			write_case_statement(wrt, nd);
			break;

		case STMT_DEFAULT:
			write_default_statement(wrt, nd);
			break;

		case STMT_COMPOUND:
			write_compound_statement(wrt, nd);
			break;

		case STMT_NULL:
			write_null_statement(wrt, nd);
			break;

		case STMT_IF:
			write_if_statement(wrt, nd);
			break;

		case STMT_SWITCH:
			write_switch_statement(wrt, nd);
			break;

		case STMT_WHILE:
			write_while_statement(wrt, nd);
			break;

		case STMT_DO:
			write_do_statement(wrt, nd);
			break;

		case STMT_FOR:
			write_for_statement(wrt, nd);
			break;

		case STMT_CONTINUE:
			write_continue_statement(wrt, nd);
			break;

		case STMT_BREAK:
			write_break_statement(wrt, nd);
			break;

		case STMT_RETURN:
			write_return_statement(wrt, nd);
			break;

		default:
			break;
	}

	wrt->indent--;
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
	wrt->indent = 0;

	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node declaration = translation_unit_get_declaration(nd, i);
		write_declaration(wrt, &declaration);
	}
}


/*
 *	 ______     ______     _____     ______     ______
 *	/\  ___\   /\  __ \   /\  __-.  /\  ___\   /\  ___\
 *	\ \ \____  \ \ \/\ \  \ \ \/\ \ \ \  __\   \ \___  \
 *	 \ \_____\  \ \_____\  \ \____-  \ \_____\  \/\_____\
 *	  \/_____/   \/_____/   \/____/   \/_____/   \/_____/
 */


static size_t elem_get_name(const instruction_t elem, const size_t num, char *const buffer)
{
	if (buffer == NULL)
	{
		return 0;
	}

	size_t argc = 0;
	bool was_switch = false;

	switch (elem)
	{
		case IC_COPY01:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY01");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY10:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY10");
					break;
				case 1:
					sprintf(buffer, "displright");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY11:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY11");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY0ST:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0ST");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY1ST:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1ST");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY0ST_ASSIGN:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0STASS");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPY1ST_ASSIGN:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1STASS");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_COPYST:
			argc = 3;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPYST");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
				case 3:
					sprintf(buffer, "length1");
					break;
			}
			break;

		case IC_CALL1:
			sprintf(buffer, "CALL1");
			break;
		case IC_CALL2:
			argc = 1;
			sprintf(buffer, "CALL2");
			break;
		case IC_CREATE:
			sprintf(buffer, "TCREATE");
			break;
		case IC_EXIT:
			sprintf(buffer, "TEXIT");
			break;
		case IC_MSG_SEND:
			sprintf(buffer, "TMSGSEND");
			break;
		case IC_MSG_RECEIVE:
			sprintf(buffer, "TMSGRECEIVE");
			break;
		case IC_JOIN:
			sprintf(buffer, "TJOIN");
			break;
		case IC_SLEEP:
			sprintf(buffer, "TSLEEP");
			break;
		case IC_SEM_CREATE:
			sprintf(buffer, "TSEMCREATE");
			break;
		case IC_SEM_WAIT:
			sprintf(buffer, "TSEMWAIT");
			break;
		case IC_SEM_POST:
			sprintf(buffer, "TSEMPOST");
			break;
		case IC_INIT:
			sprintf(buffer, "INITC");
			break;
		case IC_DESTROY:
			sprintf(buffer, "DESTROYC");
			break;
		case IC_GETNUM:
			sprintf(buffer, "GETNUMC");
			break;

		case IC_PRINT:
			argc = 1;
			sprintf(buffer, "PRINT");
			break;
		case IC_PRINTID:
			argc = 1;
			sprintf(buffer, "PRINTID");
			break;
		case IC_PRINTF:
			argc = 1;
			sprintf(buffer, "PRINTF");
			break;
		case IC_GETID:
			argc = 1;
			sprintf(buffer, "GETID");
			break;

		case IC_ABS:
			sprintf(buffer, "ABS");
			break;
		case IC_ABSI:
			sprintf(buffer, "ABSI");
			break;
		case IC_SQRT:
			sprintf(buffer, "SQRT");
			break;
		case IC_EXP:
			sprintf(buffer, "EXP");
			break;
		case IC_SIN:
			sprintf(buffer, "SIN");
			break;
		case IC_COS:
			sprintf(buffer, "COS");
			break;
		case IC_LOG:
			sprintf(buffer, "LOG");
			break;
		case IC_LOG10:
			sprintf(buffer, "LOG10");
			break;
		case IC_ASIN:
			sprintf(buffer, "ASIN");
			break;
		case IC_RAND:
			sprintf(buffer, "RAND");
			break;
		case IC_ROUND:
			sprintf(buffer, "ROUND");
			break;

		case IC_STRCPY:
			sprintf(buffer, "STRCPY");
			break;
		case IC_STRNCPY:
			sprintf(buffer, "STRNCPY");
			break;
		case IC_STRCAT:
			sprintf(buffer, "STRCAT");
			break;
		case IC_STRNCAT:
			sprintf(buffer, "STRNCAT");
			break;
		case IC_STRCMP:
			sprintf(buffer, "STRCMP");
			break;
		case IC_STRNCMP:
			sprintf(buffer, "STRNCMP");
			break;
		case IC_STRSTR:
			sprintf(buffer, "STRSTR");
			break;
		case IC_STRLEN:
			sprintf(buffer, "STRLENC");
			break;

		case IC_BEG_INIT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "BEGINIT");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case IC_STRUCT_WITH_ARR:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "STRUCTWITHARR");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "iniproc");
					break;
			}
			break;
		case IC_DEFARR:
			argc = 7;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "DEFARR");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "iniproc");
					break;
				case 5:
					sprintf(buffer, "usual");
					break;
				case 6:
					sprintf(buffer, "all");
					break;
				case 7:
					sprintf(buffer, "instruct");
					break;
			}
			break;
		case IC_ARR_INIT:
			argc = 4;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "ARRINIT");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "usual");
					break;
			}
			break;
		case IC_LI:
			argc = 1;
			sprintf(buffer, "LI");
			break;
		case IC_LID:
			argc = 2;
			sprintf(buffer, "LID");
			break;
		case IC_LOAD:
			argc = 1;
			sprintf(buffer, "LOAD");
			break;
		case IC_LOADD:
			argc = 1;
			sprintf(buffer, "LOADD");
			break;
		case IC_LAT:
			sprintf(buffer, "L@");
			break;
		case IC_LATD:
			sprintf(buffer, "L@f");
			break;
		case IC_LA:
			argc = 1;
			sprintf(buffer, "LA");
			break;

		case IC_STOP:
			sprintf(buffer, "STOP");
			break;
		case IC_RETURN_VAL:
			argc = 1;
			sprintf(buffer, "RETURNVAL");
			break;
		case IC_RETURN_VOID:
			sprintf(buffer, "RETURNVOID");
			break;
		case IC_B:
			argc = 1;
			sprintf(buffer, "B");
			break;
		case IC_BE0:
			argc = 1;
			sprintf(buffer, "BE0");
			break;
		case IC_BNE0:
			argc = 1;
			sprintf(buffer, "BNE0");
			break;
		case IC_SLICE:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SLICE");
					break;
				case 1:
					sprintf(buffer, "d");
					break;
			}
			break;
		case IC_SELECT:
			argc = 1;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SELECT");
					break;
				case 1:
					sprintf(buffer, "field_displ");
					break;
			}
			break;
		case IC_FUNC_BEG:
			argc = 2;
			was_switch = true;
			switch (num)
			{
				case 0:
					sprintf(buffer, "FUNCBEG");
					break;
				case 1:
					sprintf(buffer, "maxdispl");
					break;
				case 2:
					sprintf(buffer, "pc");
					break;
			}
			break;

		case IC_LOG_OR:
			sprintf(buffer, "||");
			break;
		case IC_LOG_AND:
			sprintf(buffer, "&&");
			break;

		case IC_OR_ASSIGN:
			argc = 1;
			sprintf(buffer, "|=");
			break;
		case IC_OR_ASSIGN_AT:
			sprintf(buffer, "|=@");
			break;
		case IC_OR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "|=V");
			break;
		case IC_OR_ASSIGN_AT_V:
			sprintf(buffer, "|=@V");
			break;
		case IC_OR:
			sprintf(buffer, "|");
			break;

		case IC_XOR_ASSIGN:
			argc = 1;
			sprintf(buffer, "^=");
			break;
		case IC_XOR_ASSIGN_AT:
			sprintf(buffer, "^=@");
			break;
		case IC_XOR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "^=V");
			break;
		case IC_XOR_ASSIGN_AT_V:
			sprintf(buffer, "^=@V");
			break;
		case IC_XOR:
			sprintf(buffer, "^");
			break;

		case IC_AND_ASSIGN:
			argc = 1;
			sprintf(buffer, "&=");
			break;
		case IC_AND_ASSIGN_AT:
			sprintf(buffer, "&=@");
			break;
		case IC_AND_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "&=V");
			break;
		case IC_AND_ASSIGN_AT_V:
			sprintf(buffer, "&=@V");
			break;
		case IC_AND:
			sprintf(buffer, "&");
			break;

		case IC_EQ:
			sprintf(buffer, "==");
			break;
		case IC_NE:
			sprintf(buffer, "!=");
			break;
		case IC_LT:
			sprintf(buffer, "<");
			break;
		case IC_GT:
			sprintf(buffer, ">");
			break;
		case IC_LE:
			sprintf(buffer, "<=");
			break;
		case IC_GE:
			sprintf(buffer, ">=");
			break;
		case IC_EQ_R:
			sprintf(buffer, "==f");
			break;
		case IC_NE_R:
			sprintf(buffer, "!=f");
			break;
		case IC_LT_R:
			sprintf(buffer, "<f");
			break;
		case IC_GT_R:
			sprintf(buffer, ">f");
			break;
		case IC_LE_R:
			sprintf(buffer, "<=f");
			break;
		case IC_GE_R:
			sprintf(buffer, ">=f");
			break;

		case IC_SHR_ASSIGN:
			argc = 1;
			sprintf(buffer, ">>=");
			break;
		case IC_SHR_ASSIGN_AT:
			sprintf(buffer, ">>=@");
			break;
		case IC_SHR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, ">>=V");
			break;
		case IC_SHR_ASSIGN_AT_V:
			sprintf(buffer, ">>=@V");
			break;
		case IC_SHR:
			sprintf(buffer, ">>");
			break;

		case IC_SHL_ASSIGN:
			argc = 1;
			sprintf(buffer, "<<=");
			break;
		case IC_SHL_ASSIGN_AT:
			sprintf(buffer, "<<=@");
			break;
		case IC_SHL_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "<<=V");
			break;
		case IC_SHL_ASSIGN_AT_V:
			sprintf(buffer, "<<=@V");
			break;
		case IC_SHL:
			sprintf(buffer, "<<");
			break;

		case IC_ASSIGN:
			argc = 1;
			sprintf(buffer, "=");
			break;
		case IC_ASSIGN_AT:
			sprintf(buffer, "=@");
			break;
		case IC_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "=V");
			break;
		case IC_ASSIGN_AT_V:
			sprintf(buffer, "=@V");
			break;

		case IC_ADD_ASSIGN:
			argc = 1;
			sprintf(buffer, "+=");
			break;
		case IC_ADD_ASSIGN_AT:
			sprintf(buffer, "+=@");
			break;
		case IC_ADD_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "+=V");
			break;
		case IC_ADD_ASSIGN_AT_V:
			sprintf(buffer, "+=@V");
			break;
		case IC_ADD:
			sprintf(buffer, "+");
			break;

		case IC_SUB_ASSIGN:
			argc = 1;
			sprintf(buffer, "-=");
			break;
		case IC_SUB_ASSIGN_AT:
			sprintf(buffer, "-=@");
			break;
		case IC_SUB_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "-=V");
			break;
		case IC_SUB_ASSIGN_AT_V:
			sprintf(buffer, "-=@V");
			break;
		case IC_SUB:
			sprintf(buffer, "-");
			break;

		case IC_MUL_ASSIGN:
			argc = 1;
			sprintf(buffer, "*=");
			break;
		case IC_MUL_ASSIGN_AT:
			sprintf(buffer, "*=@");
			break;
		case IC_MUL_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "*=V");
			break;
		case IC_MUL_ASSIGN_AT_V:
			sprintf(buffer, "*=@V");
			break;
		case IC_MUL:
			sprintf(buffer, "*");
			break;

		case IC_DIV_ASSIGN:
			argc = 1;
			sprintf(buffer, "/=");
			break;
		case IC_DIV_ASSIGN_AT:
			sprintf(buffer, "/=@");
			break;
		case IC_DIV_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "/=V");
			break;
		case IC_DIV_ASSIGN_AT_V:
			sprintf(buffer, "/=@V");
			break;
		case IC_DIV:
			sprintf(buffer, "/");
			break;

		case IC_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "=f");
			break;
		case IC_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "=fV");
			break;
		case IC_ASSIGN_AT_R:
			sprintf(buffer, "=@f");
			break;
		case IC_ASSIGN_AT_R_V:
			sprintf(buffer, "=@fV");
			break;

		case IC_ADD_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "+=f");
			break;
		case IC_ADD_ASSIGN_AT_R:
			sprintf(buffer, "+=@f");
			break;
		case IC_ADD_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "+=fV");
			break;
		case IC_ADD_ASSIGN_AT_R_V:
			sprintf(buffer, "+=@fV");
			break;
		case IC_ADD_R:
			sprintf(buffer, "+f");
			break;

		case IC_SUB_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "-=f");
			break;
		case IC_SUB_ASSIGN_AT_R:
			sprintf(buffer, "-=@f");
			break;
		case IC_SUB_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "-=fV");
			break;
		case IC_SUB_ASSIGN_AT_R_V:
			sprintf(buffer, "-=@fV");
			break;
		case IC_SUB_R:
			sprintf(buffer, "-f");
			break;

		case IC_MUL_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "*=f");
			break;
		case IC_MUL_ASSIGN_AT_R:
			sprintf(buffer, "*=@f");
			break;
		case IC_MUL_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "*=fV");
			break;
		case IC_MUL_ASSIGN_AT_R_V:
			sprintf(buffer, "*=@fV");
			break;
		case IC_MUL_R:
			sprintf(buffer, "*f");
			break;

		case IC_DIV_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "/=f");
			break;
		case IC_DIV_ASSIGN_AT_R:
			sprintf(buffer, "/=@f");
			break;
		case IC_DIV_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "/=fV");
			break;
		case IC_DIV_ASSIGN_AT_R_V:
			sprintf(buffer, "/=@fV");
			break;
		case IC_DIV_R:
			sprintf(buffer, "/f");
			break;

		case IC_REM_ASSIGN:
			argc = 1;
			sprintf(buffer, "%%=");
			break;
		case IC_REM_ASSIGN_AT:
			sprintf(buffer, "%%=@");
			break;
		case IC_REM_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "%%=V");
			break;
		case IC_REM_ASSIGN_AT_V:
			sprintf(buffer, "%%=@V");
			break;
		case IC_REM:
			sprintf(buffer, "%%");
			break;
		case IC_POST_INC:
			argc = 1;
			sprintf(buffer, "POSTINC");
			break;
		case IC_POST_DEC:
			argc = 1;
			sprintf(buffer, "POSTDEC");
			break;
		case IC_PRE_INC_AT:
			sprintf(buffer, "INC@");
			break;
		case IC_PRE_DEC_AT:
			sprintf(buffer, "DEC@");
			break;
		case IC_POST_INC_AT:
			sprintf(buffer, "POSTINC@");
			break;
		case IC_POST_DEC_AT:
			sprintf(buffer, "POSTDEC@");
			break;
		case IC_PRE_INC_R:
			argc = 1;
			sprintf(buffer, "INCf");
			break;
		case IC_PRE_DEC_R:
			argc = 1;
			sprintf(buffer, "DECf");
			break;
		case IC_POST_INC_R:
			argc = 1;
			sprintf(buffer, "POSTINCf");
			break;
		case IC_POST_DEC_R:
			argc = 1;
			sprintf(buffer, "POSTDECf");
			break;
		case IC_PRE_INC_AT_R:
			sprintf(buffer, "INC@f");
			break;
		case IC_PRE_DEC_AT_R:
			sprintf(buffer, "DEC@f");
			break;
		case IC_POST_INC_AT_R:
			sprintf(buffer, "POSTINC@f");
			break;
		case IC_POST_DEC_AT_R:
			sprintf(buffer, "POSTDEC@f");
			break;
		case IC_PRE_INC_V:
			argc = 1;
			sprintf(buffer, "INCV");
			break;
		case IC_PRE_DEC_V:
			argc = 1;
			sprintf(buffer, "DECV");
			break;
		case IC_POST_INC_V:
			argc = 1;
			sprintf(buffer, "POSTINCV");
			break;
		case IC_POST_DEC_V:
			argc = 1;
			sprintf(buffer, "POSTDECV");
			break;
		case IC_PRE_INC_AT_V:
			sprintf(buffer, "INC@V");
			break;
		case IC_PRE_DEC_AT_V:
			sprintf(buffer, "DEC@V");
			break;
		case IC_POST_INC_AT_V:
			sprintf(buffer, "POSTINC@V");
			break;
		case IC_POST_DEC_AT_V:
			sprintf(buffer, "POSTDEC@V");
			break;
		case IC_PRE_INC_R_V:
			argc = 1;
			sprintf(buffer, "INCfV");
			break;
		case IC_PRE_DEC_R_V:
			argc = 1;
			sprintf(buffer, "DECfV");
			break;
		case IC_POST_INC_R_V:
			argc = 1;
			sprintf(buffer, "POSTINCfV");
			break;
		case IC_POST_DEC_R_V:
			argc = 1;
			sprintf(buffer, "POSTDECfV");
			break;
		case IC_PRE_INC_AT_R_V:
			sprintf(buffer, "INC@fV");
			break;
		case IC_PRE_DEC_AT_R_V:
			sprintf(buffer, "DEC@fV");
			break;
		case IC_POST_INC_AT_R_V:
			sprintf(buffer, "POSTINC@fV");
			break;
		case IC_POST_DEC_AT_R_V:
			sprintf(buffer, "POSTDEC@fV");
			break;
		case IC_NOP:
			sprintf(buffer, "NOP");
			break;
		case IC_WIDEN:
			sprintf(buffer, "WIDEN");
			break;
		case IC_WIDEN1:
			sprintf(buffer, "WIDEN1");
			break;
		case IC_DUPLICATE:
			sprintf(buffer, "DUPLICATE");
			break;
		case IC_COPY00:
			argc = 3;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY00");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "displright");
					break;
				case 3:
					sprintf(buffer, "length");
					break;
			}
			break;
		case IC_PRE_INC:
			argc = 1;
			sprintf(buffer, "INC");
			break;
		case IC_PRE_DEC:
			argc = 1;
			sprintf(buffer, "DEC");
			break;
		case IC_NOT:
			sprintf(buffer, "BITNOT");
			break;
		case IC_LOG_NOT:
			sprintf(buffer, "NOT");
			break;

		case IC_UNMINUS:
			sprintf(buffer, "UNMINUS");
			break;
		case IC_UNMINUS_R:
			sprintf(buffer, "UNMINUSf");
			break;
		default:
			sprintf(buffer, "%i", elem);
			break;
	}

	if ((num != 0 && !was_switch) || argc < num)
	{
		buffer[0] = '\0';
	}
	return argc;
}

static size_t write_instruction(universal_io *const io, const vector *const table, size_t i)
{
	const instruction_t type = (instruction_t)vector_get(table, i++);

	char buffer[MAX_ELEM_SIZE];
	size_t argc = elem_get_name(type, 0, buffer);
	uni_printf(io, "%s", buffer);

	if (type == IC_LID)
	{
		//uni_printf(io, " %f\n", vector_get_double(table, i));

		const int64_t fst = (int64_t)vector_get(table, i) & 0x00000000ffffffff;
		const int64_t snd = (int64_t)vector_get(table, i + 1) & 0x00000000ffffffff;
		const int64_t num64 = (snd << 32) | fst;

		double num;
		memcpy(&num, &num64, sizeof(double));
		uni_printf(io, " %f\n", num);
		return i + 2;
	}

	for (size_t j = 1; j <= argc; j++)
	{
		elem_get_name(type, j, buffer);

		if (buffer[0] != '\0')
		{
			uni_printf(io, " %s=", buffer);
		}

		uni_printf(io, " %" PRIitem, vector_get(table, i++));
	}

	uni_printf(io, "\n");
	return i;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void write_tree(const char *const path, syntax *const sx)
{
	universal_io io = io_create();
	if (path == NULL || sx == NULL || out_set_file(&io, path))
	{
		return;
	}

	writer wrt = { .sx = sx, .io = &io };

	const node root = node_get_root(&sx->tree);
	write_translation_unit(&wrt, &root);

	io_erase(&io);
}


int write_type_spelling(const syntax *const sx, const item_t type, char *const buffer)
{
	const type_t type_class = type_get_class(sx, type);
	switch (type_class)
	{
		case TYPE_VARARG:
			return sprintf(buffer, "...");

		case TYPE_NULL_POINTER:
			return sprintf(buffer, "nullptr");

		case TYPE_FILE:
			return sprintf(buffer, "FILE");

		case TYPE_VOID:
			return sprintf(buffer, "void");

		case TYPE_BOOLEAN:
			return sprintf(buffer, "bool");

		case TYPE_FLOATING:
			return sprintf(buffer, "double");

		case TYPE_CHARACTER:
			return sprintf(buffer, "char");

		case TYPE_INTEGER:
			return sprintf(buffer, "int");

		case TYPE_UNDEFINED:
			return sprintf(buffer, "undefined type");

		case TYPE_FUNCTION:
		{
			int index = write_type_spelling(sx, type_function_get_return_type(sx, type), buffer);
			index += sprintf(&buffer[index], " (");

			const size_t parameter_amount = type_function_get_parameter_amount(sx, type);
			for (size_t i = 0; i < parameter_amount; i++)
			{
				const item_t parameter_type = type_function_get_parameter_type(sx, type, i);

				index += write_type_spelling(sx, parameter_type, &buffer[index]);
				if (i != parameter_amount - 1)
				{
					index += sprintf(&buffer[index], ", ");
				}
			}

			index += sprintf(&buffer[index], ")");
			return index;
		}

		case TYPE_MSG_INFO:
		case TYPE_STRUCTURE:
		{
			int index = sprintf(buffer, "struct { ");

			const size_t member_amount = type_structure_get_member_amount(sx, type);
			for (size_t i = 0; i < member_amount; i++)
			{
				const item_t member_type = type_structure_get_member_type(sx, type, i);
				const size_t member_repr = type_structure_get_member_name(sx, type, i);

				index += write_type_spelling(sx, member_type, &buffer[index]);
				index += sprintf(&buffer[index], " %s; ", repr_get_name(sx, member_repr));
			}

			index += sprintf(&buffer[index], "}");
			return index;
		}

		case TYPE_ARRAY:
		{
			const item_t element_type = type_array_get_element_type(sx, type);
			int index = write_type_spelling(sx, element_type, buffer);
			index += sprintf(&buffer[index], "[]");
			return index;
		}

		case TYPE_POINTER:
		{
			const item_t element_type = type_pointer_get_element_type(sx, type);
			int index = write_type_spelling(sx, element_type, buffer);
			index += sprintf(&buffer[index], "*");
			return index;
		}

		case TYPE_CONST:
		{
			const item_t element_type = type_const_get_unqualified_type(sx, type);
			if (type_is_pointer(sx, element_type))
			{
				int index = write_type_spelling(sx, element_type, buffer);
				index += sprintf(&buffer[index], " const");
				return index;
			}
			else
			{
				int index = sprintf(buffer, "const ");
				index += write_type_spelling(sx, element_type, &buffer[index]);
				return index; 
			}
		}

		case TYPE_REFERENCE:
		{
			const item_t element_type = type_reference_get_element_type(sx, type);
			int index = write_type_spelling(sx, element_type, buffer);
			index += sprintf(&buffer[index], "&");
			return index;
		}

		default:
			return 0;
	}
}

void write_codes(const char *const path, const vector *const memory)
{
	universal_io io = io_create();
	if (path == NULL || !vector_is_correct(memory) || out_set_file(&io, path))
	{
		return;
	}

	uni_printf(&io, "mem\n");
	size_t i = 0;
	while (i < vector_size(memory))
	{
		uni_printf(&io, "pc %zu) ", i);
		i = write_instruction(&io, memory, i);
	}

	io_erase(&io);
}
