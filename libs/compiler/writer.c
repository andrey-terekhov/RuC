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


/** AST writer */
typedef struct writer
{
	const syntax *sx;					/**< Syntax structure */
	universal_io *io;					/**< Output file */
} writer;

static void write_expression(writer *const wrt, const node *const nd);
static void write_statement(writer *const wrt, const node *const nd);


//===----------------------------------------------------------------------===//
//                            Expression Writing                              //
//===----------------------------------------------------------------------===//

/**
 *	Write identifier expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_identifier_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write literal expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_literal_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write subscript expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_subscript_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write call expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_call_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write member expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_member_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write unary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_unary_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write binary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_binary_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write ternary expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_ternary_expression(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write expression list
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_expression_list(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write expression
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_expression(writer *const wrt, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			write_identifier_expression(wrt, nd);
			return;

		case EXPR_LITERAL:
			write_literal_expression(wrt, nd);
			return;

		case EXPR_SUBSCRIPT:
			write_subscript_expression(wrt, nd);
			return;

		case EXPR_CALL:
			write_call_expression(wrt, nd);
			return;

		case EXPR_MEMBER:
			write_member_expression(wrt, nd);
			return;

		case EXPR_UNARY:
			write_unary_expression(wrt, nd);
			return;

		case EXPR_BINARY:
			write_binary_expression(wrt, nd);
			return;

		case EXPR_TERNARY:
			write_ternary_expression(wrt, nd);
			return;

		case EXPR_LIST:
			write_expression_list(wrt, nd);
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
 */
static void write_variable_declaration(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write type declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_type_declaration(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write function declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_function_declaration(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write declaration
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_declaration(writer *const wrt, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			write_variable_declaration(wrt, nd);
			return;

		case DECL_TYPE:
			write_type_declaration(wrt, nd);
			return;

		case DECL_FUNC:
			write_function_declaration(wrt, nd);
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
 */
static void write_labeled_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write case statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_case_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write default statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_default_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write compound statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_compound_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write null statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_null_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
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
	(void)wrt;
	(void)nd;
}

/**
 *	Write switch statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_switch_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write while statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_while_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write do statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_do_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write for statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_for_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write goto statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_goto_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
	(void)nd;
}

/**
 *	Write continue statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST	
 */
static void write_continue_statement(writer *const wrt, const node *const nd)
{
	(void)wrt;
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
	(void)wrt;
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
	(void)wrt;
	(void)nd;
}

/**
 *	Write statement
 *
 *	@param	wrt			Writer
 *	@param	nd			Node in AST
 */
static void write_statement(writer *const wrt, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			write_declaration(wrt, nd);
			return;

		case STMT_LABEL:
			write_labeled_statement(wrt, nd);
			return;

		case STMT_CASE:
			write_case_statement(wrt, nd);
			return;

		case STMT_DEFAULT:
			write_default_statement(wrt, nd);
			return;

		case STMT_COMPOUND:
			write_compound_statement(wrt, nd);
			return;

		case STMT_EXPR:
			write_expression(wrt, nd);
			return;

		case STMT_NULL:
			write_null_statement(wrt, nd);
			return;

		case STMT_IF:
			write_if_statement(wrt, nd);
			return;

		case STMT_SWITCH:
			write_switch_statement(wrt, nd);
			return;

		case STMT_WHILE:
			write_while_statement(wrt, nd);
			return;

		case STMT_DO:
			write_do_statement(wrt, nd);
			return;

		case STMT_FOR:
			write_for_statement(wrt, nd);
			return;

		case STMT_GOTO:
			write_goto_statement(wrt, nd);
			return;

		case STMT_CONTINUE:
			write_continue_statement(wrt, nd);
			return;

		case STMT_BREAK:
			write_break_statement(wrt, nd);
			return;

		case STMT_RETURN:
			write_return_statement(wrt, nd);
			return;
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
	(void)wrt;
	(void)nd;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void write_AST(const char *const path, const syntax *const sx, const node *root)
{
	universal_io io = io_create();
	if (path == NULL || sx == NULL || out_set_file(&io, path))
	{
		return;
	}

	writer wrt = { .sx = sx, .io = &io };
	write_translation_unit(&wrt, root);
	//wrt_clear(&wrt);
}
