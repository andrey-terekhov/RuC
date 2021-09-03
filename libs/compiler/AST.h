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

#pragma once

#include "operations.h"
#include "tree.h"
#include "syntax.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Value category */
typedef enum CATEGORY
{
	LVALUE,				/**< An expression that designates an object */
	RVALUE,				/**< An expression detached from any specific storage */
} category_t;

/** Expression class */
typedef enum EXPRESSION
{
	EXPR_IDENTIFIER,	/**< Identifier expression */
	EXPR_LITERAL,		/**< Literal expression */
	EXPR_SUBSCRIPT,		/**< Subscript expression */
	EXPR_CALL,			/**< Call expression */
	EXPR_MEMBER,		/**< Member expression */
	EXPR_UNARY,			/**< Unary expression */
	EXPR_BINARY,		/**< Binary expression */
	EXPR_TERNARY,		/**< Ternary expression */
	EXPR_LIST,			/**< Expression list */
} expression_t;

/** Statement class */
typedef enum STATEMENT
{
	STMT_DECL,			/**< Declaration statement */
	STMT_LABEL,			/**< Labeled statement */
	STMT_CASE,			/**< Case statement */
	STMT_DEFAULT,		/**< Default statement */
	STMT_COMPOUND,		/**< Compound statement */
	STMT_EXPR,			/**< Expression statement */
	STMT_NULL,			/**< Null statement */
	STMT_IF,			/**< If statement */
	STMT_SWITCH,		/**< Switch statement */
	STMT_WHILE,			/**< While statement */
	STMT_DO,			/**< Do statement */
	STMT_FOR,			/**< For statement */
	STMT_GOTO,			/**< Goto statement */
	STMT_CONTINUE,		/**< Continue statement */
	STMT_BREAK,			/**< Break statement */
	STMT_RETURN,		/**< Return statement */
	STMT_PRINTF,		/**< Printf statement */
} statement_t;

/** Declaration class */
typedef enum DECLARATION
{
	DECL_VAR,			/**< Variable declaration */
	DECL_FUNC,			/**< Function declaration */
	DECL_TYPE,			/**< Type declaration */
} declaration_t;


/**
 *	Build a broken node
 *
 *	@return	Broken node
 */
inline node node_broken()
{
	return node_load(NULL, SIZE_MAX);
}


/**
 *	Get expression class
 *
 *	@param	nd		Expression
 *
 *	@return	Expression class
 */
expression_t expression_get_class(const node *const nd);

/**
 *	Get expression type
 *
 *	@param	nd		Expression
 *
 *	@return	Expression type
 */
inline item_t expression_get_type(const node *const nd)
{
	return node_get_arg(nd, 0);
}

/**
 *	Check if expression is lvalue
 *
 *	@param	nd		Expression
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool expression_is_lvalue(const node *const nd)
{
	return node_get_arg(nd, 1) == LVALUE;
}

/**
 *	Get expression location
 *
 *	@param	nd		Expression
 *
 *	@return	Expression location
 */
inline location expression_get_location(const node *const nd)
{
	const size_t argc = node_get_argc(nd);
	return (location){ (size_t)node_get_arg(nd, argc - 2), (size_t)node_get_arg(nd, argc - 1) };
}


/**
 *	Get id of identifier expression
 *
 *	@param	nd		Identifier expression
 *
 *	@return	Id
 */
inline size_t expression_identifier_get_id(const node *const nd)
{
	return node_get_type(nd) == OP_IDENTIFIER ? (size_t)node_get_arg(nd, 2) : SIZE_MAX;
}


/**
 *	Get integer value of literal expression
 *
 *	@param	nd		Literal expression
 *
 *	@return	Integer value
 */
inline int expression_literal_get_integer(const node *const nd)
{
	return node_get_type(nd) == OP_LITERAL ? (int)node_get_arg(nd, 2) : INT_MAX;
}

/**
 *	Get floating value of literal expression
 *
 *	@param	nd		Literal expression
 *
 *	@return	Floating value
 */
inline double expression_literal_get_floating(const node *const nd)
{
	return node_get_type(nd) == OP_LITERAL ? node_get_arg_double(nd, 2) : DBL_MAX;
}

/**
 *	Get string index of literal expression
 *
 *	@param	nd		Literal expression
 *
 *	@return	String index
 */
inline size_t expression_literal_get_string(const node *const nd)
{
	return node_get_type(nd) == OP_LITERAL ? (size_t)node_get_arg(nd, 2) : SIZE_MAX;
}


/**
 *	Get base expression of subscript expression
 *
 *	@param	nd		Subscript expression
 *
 *	@return	Base expression
 */
inline node expression_subscript_get_base(const node *const nd)
{
	return node_get_type(nd) == OP_SLICE ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get index expression of subscript expression
 *
 *	@param	nd		Subscript expression
 *
 *	@return	Index expression
 */
inline node expression_subscript_get_index(const node *const nd)
{
	return node_get_type(nd) == OP_SLICE ? node_get_child(nd, 1) : node_broken();
}


/**
 *	Get called expression of call expression
 *
 *	@param	nd		Call expression
 *
 *	@return	Called expression
 */
inline node expression_call_get_callee(const node *const nd)
{
	return node_get_type(nd) == OP_CALL ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get argument of call expression by index
 *
 *	@param	nd		Call expression
 *	@param	index	Argument index
 *
 *	@return	Argument
 */
inline node expression_call_get_argument(const node *const nd, const size_t index)
{
	return node_get_type(nd) == OP_CALL ? node_get_child(nd, 1 + index) : node_broken();
}


/**
 *	Get base expression of member expression
 *
 *	@param	nd		Member expression
 *
 *	@return	Base expression
 */
inline node expression_member_get_base(const node *const nd)
{
	return node_get_type(nd) == OP_SELECT ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get member displacement of member expression
 *
 *	@param	nd		Member expression
 *
 *	@return	Member displacement
 */
inline size_t expression_member_get_member_displ(const node *const nd)
{
	return node_get_type(nd) == OP_SELECT ? (size_t)node_get_arg(nd, 2) : SIZE_MAX;
}


/**
 *	Get operator of unary expression
 *
 *	@param	nd		Unary expression
 *
 *	@return	Operator
 */
inline unary_t expression_unary_get_operator(const node *const nd)
{
	return node_get_type(nd) == OP_UNARY ? (unary_t)node_get_arg(nd, 2) : INT_MAX;
}

/**
 *	Get operand of unary expression
 *
 *	@param	nd		Unary expression
 *
 *	@return	Operand
 */
inline node expression_unary_get_operand(const node *const nd)
{
	return node_get_type(nd) == OP_UNARY ? node_get_child(nd, 0) : node_broken();
}


/**
 *	Get operator of binary expression
 *
 *	@param	nd		Binary expression
 *
 *	@return	Operator
 */
inline binary_t expression_binary_get_operator(const node *const nd)
{
	return node_get_type(nd) == OP_BINARY ? (binary_t)node_get_arg(nd, 2) : INT_MAX;
}

/**
 *	Get LHS of binary expression
 *
 *	@param	nd		Binary expression
 *
 *	@return	LHS of binary expression
 */
inline node expression_binary_get_LHS(const node *const nd)
{
	return node_get_type(nd) == OP_BINARY ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get RHS of binary expression
 *
 *	@param	nd		Binary expression
 *
 *	@return	RHS of binary expression
 */
inline node expression_binary_get_RHS(const node *const nd)
{
	return node_get_type(nd) == OP_BINARY ? node_get_child(nd, 1) : node_broken();
}


/**
 *	Get condition of ternary expression
 *
 *	@param	nd		Ternary expression
 *
 *	@return	Condition
 */
inline node expression_ternary_get_condition(const node *const nd)
{
	return node_get_type(nd) == OP_TERNARY ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get LHS of ternary expression
 *
 *	@param	nd		Ternary expression
 *
 *	@return	LHS of ternary expression
 */
inline node expression_ternary_get_LHS(const node *const nd)
{
	return node_get_type(nd) == OP_TERNARY ? node_get_child(nd, 1) : node_broken();
}

/**
 *	Get RHS of ternary expression
 *
 *	@param	nd		Ternary expression
 *
 *	@return	RHS of ternary expression
 */
inline node expression_ternary_get_RHS(const node *const nd)
{
	return node_get_type(nd) == OP_TERNARY ? node_get_child(nd, 2) : node_broken();
}


/**
 *	Get size of expression list
 *
 *	@param	nd		Expression list
 *
 *	@return	Size
 */
inline size_t expression_list_get_size(const node *const nd)
{
	return node_get_type(nd) == OP_LIST ? node_get_amount(nd) : 0;
}

/**
 *	Get subexpression of expression list by index
 *
 *	@param	nd		Expression list
 *	@param	index	Subexpression index
 *
 *	@return	Expression
 */
inline node expression_list_get_subexpr(const node *const nd, const size_t index)
{
	return node_get_type(nd) == OP_LIST ? node_get_child(nd, index) : node_broken();
}


/**
 *	Get statement class
 *
 *	@param	nd		Statement
 *
 *	@return	Statement class
 */
statement_t statement_get_class(const node *const nd);


/**
 *	Get label id of labeled statement
 *
 *	@param	nd		Labeled statement
 *
 *	@return	Label id
 */
inline size_t statement_labeled_get_label(const node *const nd)
{
	return node_get_type(nd) == OP_LABEL ? (size_t)node_get_arg(nd, 0) : SIZE_MAX;
}

/**
 *	Get substatement of labeled statement
 *
 *	@param	nd		Labeled statement
 *
 *	@return	Substatement
 */
inline node statement_labeled_get_substmt(const node *const nd)
{
	return node_get_type(nd) == OP_LABEL ? node_get_child(nd, 0) : node_broken();
}


/**
 *	Get size of compound statement
 *
 *	@param	nd		Compound statement
 *
 *	@return	Size
 */
inline size_t statement_compound_get_size(const node *const nd)
{
	return node_get_type(nd) == OP_BLOCK ? node_get_amount(nd) : SIZE_MAX;
}

/**
 *	Get substatement of compound statement by index
 *
 *	@param	nd		Compound statement
 *	@param	index	Substatement index
 *
 *	@return	Substatement
 */
inline node statement_compound_get_substmt(const node *const nd, const size_t index)
{
	return node_get_type(nd) == OP_BLOCK ? node_get_child(nd, index) : node_broken();
}


/**
 *	Check if if statement has else-substatement
 *
 *	@param	nd		If statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool statement_if_has_else_substmt(const node *const nd)
{
	return node_get_type(nd) == OP_IF ? node_get_arg(nd, 0) != 0 : false;
}

/**
 *	Get condition of if statement
 *
 *	@param	nd		If statement
 *
 *	@return	Condition
 */
inline node statement_if_get_condition(const node *const nd)
{
	return node_get_type(nd) == OP_IF ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get then-substatement of if statement
 *
 *	@param	nd		If statement
 *
 *	@return	Then-substatement
 */
inline node statement_if_get_then_substmt(const node *const nd)
{
	return node_get_type(nd) == OP_IF ? node_get_child(nd, 1) : node_broken();
}

/**
 *	Get else-substatement of if statement
 *
 *	@param	nd		If statement
 *
 *	@return	Else-substatement
 */
inline node statement_if_get_else_substmt(const node *const nd)
{
	return statement_if_has_else_substmt(nd) ? node_get_child(nd, 2) : node_broken();
}


/**
 *	Get condition of while statement
 *
 *	@param	nd		While statement
 *
 *	@return	Condition
 */
inline node statement_while_get_condition(const node *const nd)
{
	return node_get_type(nd) == OP_WHILE ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get substatement of while statement
 *
 *	@param	nd		While statement
 *
 *	@return	Substatement
 */
inline node statement_while_get_body(const node *const nd)
{
	return node_get_type(nd) == OP_WHILE ? node_get_child(nd, 1) : node_broken();
}


/**
 *	Get condition of do statement
 *
 *	@param	nd		Do statement
 *
 *	@return	Condition
 */
inline node statement_do_get_condition(const node *const nd)
{
	return node_get_type(nd) == OP_DO ? node_get_child(nd, 1) : node_broken();
}

/**
 *	Get substatement of do statement
 *
 *	@param	nd		Do statement
 *
 *	@return	Substatement
 */
inline node statement_do_get_body(const node *const nd)
{
	return node_get_type(nd) == OP_DO ? node_get_child(nd, 0) : node_broken();
}


/**
 *	Check if for statement has inition
 *
 *	@param	nd		For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool statement_for_has_inition(const node *const nd)
{
	return node_get_type(nd) == OP_FOR ? node_get_arg(nd, 0) != 0 : false;
}

/**
 *	Check if for statement has condition
 *
 *	@param	nd		For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool statement_for_has_condition(const node *const nd)
{
	return node_get_type(nd) == OP_FOR ? node_get_arg(nd, 1) != 0 : false;
}

/**
 *	Check if for statement has increment
 *
 *	@param	nd		For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool statement_for_has_increment(const node *const nd)
{
	return node_get_type(nd) == OP_FOR ? node_get_arg(nd, 2) != 0 : false;
}

/**
 *	Get inition of for statement
 *
 *	@param	nd		For statement
 *
 *	@return	Inition
 */
inline node statement_for_get_inition(const node *const nd)
{
	return statement_for_has_inition(nd) ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get condition of for statement
 *
 *	@param	nd		For statement
 *
 *	@return	condition
 */
inline node statement_for_get_condition(const node *const nd)
{
	return statement_for_has_condition(nd)
		? node_get_child(nd, statement_for_has_inition(nd) ? 1 : 0)
		: node_broken();
}

/**
 *	Get increment of for statement
 *
 *	@param	nd		For statement
 *
 *	@return	Increment
 */
inline node statement_for_get_increment(const node *const nd)
{
	return statement_for_has_increment(nd)
		? node_get_child(nd, node_get_amount(nd) - 2)
		: node_broken();
}

/**
 *	Get substatement of for statement
 *
 *	@param	nd		For statement
 *
 *	@return	Substatement
 */
inline node statement_for_get_body(const node *const nd)
{
	return node_get_type(nd) == OP_FOR
		? node_get_child(nd, node_get_amount(nd) - 1)
		: node_broken();
}


/**
 *	Get label id of goto statement
 *
 *	@param	nd		Goto statement
 *
 *	@return	Label id
 */
inline size_t statement_goto_get_label(const node *const nd)
{
	return node_get_type(nd) == OP_GOTO ? (size_t)node_get_arg(nd, 0) : SIZE_MAX;
}


/**
 *	Check if return statement has expression
 *
 *	@param	nd		Return statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool statement_return_has_expression(const node *const nd)
{
	return node_get_type(nd) == OP_RETURN ? node_get_amount(nd) != 0 : false;
}

/**
 *	Get expression of return statement
 *
 *	@param	nd		Return statement
 *
 *	@return	Expression
 */
inline node statement_return_get_expression(const node *const nd)
{
	return statement_return_has_expression(nd) ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get argument count of printf statement
 *
 *	@param	nd		Printf statement
 *
 *	@return	Argument count
 */
inline size_t statement_printf_get_argc(const node *const nd)
{
	return node_get_type(nd) == OP_PRINTF ? node_get_amount(nd) - 1 : 0;
}

/**
 *	Get format string of printf statement
 *
 *	@param	nd		Printf statement
 *
 *	@return	Format string
 */
inline node statement_printf_get_format_str(const node *const nd)
{
	return node_get_type(nd) == OP_PRINTF ? node_get_child(nd, 0) : node_broken();
}

/**
 *	Get argument of printf statement
 *
 *	@param	nd		Printf statement
 *
 *	@return	Argument
 */
inline node statement_printf_get_argument(const node *const nd, const size_t index)
{
	return statement_printf_get_argc(nd) > index ? node_get_child(nd, 1 + index) : node_broken();
}


/**
 *	Get declaration class
 *
 *	@param	nd		Declaration
 *
 *	@return	Declaration class
 */
declaration_t declaration_get_class(const node *const nd);


/**
 *	Get variable id in variable declaration
 *
 *	@param	nd		Variable declaration
 *
 *	@return	Id
 */
inline size_t declaration_variable_get_id(const node *const nd)
{
	return node_get_type(nd) == OP_DECL_VAR ? (size_t)node_get_arg(nd, 0) : SIZE_MAX;
}

/**
 *	Check if variable declaration has initializer
 *
 *	@param	nd		Variable declaration
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool declaration_variable_has_initializer(const node *const nd)
{
	return node_get_type(nd) == OP_DECL_VAR ? node_get_arg(nd, 2) != 0 : false;
}

/**
 *	Get initializer of variable declaration
 *
 *	@param	nd		Variable declaration
 *
 *	@return	Initializer
 */
inline node declaration_variable_get_initializer(const node *const nd)
{
	return declaration_variable_has_initializer(nd) ? node_get_child(nd, node_get_amount(nd) - 1) : node_broken();
}

/**
 *	Get amount of dimenstions of variable declaration
 *
 *	@param	nd		Variable declaration
 *
 *	@return	Amount of dimenstions
 */
inline size_t declaration_variable_get_dim_amount(const node *const nd)
{
	return node_get_type(nd) == OP_DECL_VAR
		? node_get_amount(nd) - (declaration_variable_has_initializer(nd) ? 1 : 0)
		: 0;
}

/**
 *	Get size-expression of dimenstions of variable declaration by index
 *
 *	@param	nd		Variable declaration
 *	@param	index	Dimansion index
 *
 *	@return	Size-expression
 */
inline node declaration_variable_get_dim_expr(const node *const nd, const size_t index)
{
	return declaration_variable_get_dim_amount(nd) > index ? node_get_child(nd, index) : node_broken();
}


/**
 *	Get id of function in function declaration
 *
 *	@param	nd		Function declaration
 *
 *	@return Function id
 */
inline size_t declaration_function_get_id(const node *const nd)
{
	return node_get_type(nd) == OP_FUNC_DEF ? (size_t)node_get_arg(nd, 0) : SIZE_MAX;
}

/**
 *	Get parameter id in function declaration by index
 *
 *	@param	nd		Function declaration
 *	@param	index	Parameter index
 *
 *	@return Parameter id
 */
size_t declaration_function_get_param(const node *const nd, const size_t index);

/**
 *	Get body of function declaration
 *
 *	@param	nd		Function declaration
 *
 *	@return Function body
 */
inline node declaration_function_get_body(const node *const nd)
{
	return node_get_type(nd) == OP_FUNC_DEF ? node_get_child(nd, node_get_amount(nd) - 1) : node_broken();
}


/**
 *	Get number of global declarations in translation unit
 *
 *	@param	nd		Translation unit
 *
 *	@return	Number of global declarations
 */
inline size_t translation_unit_get_size(const node *const nd)
{
	return node_get_type(nd) == ITEM_MAX && node_is_correct(nd) ? node_get_amount(nd) : SIZE_MAX;
}

/**
 *	Get global declaration of translation unit by index
 *
 *	@param	nd		Translation unit
 *	@param	index	Global declaration index
 *
 *	@return	Global declaration
 */
inline node translation_unit_get_declaration(const node *const nd, const size_t index)
{
	return node_get_type(nd) == ITEM_MAX && node_is_correct(nd) ? node_get_child(nd, index) : node_broken();
}

#ifdef __cplusplus
} /* extern "C" */
#endif
