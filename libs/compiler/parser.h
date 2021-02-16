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

#include "analyzer.h"
#include "errors.h"


#ifdef __cplusplus
extern "C" {
#endif

/**	The kind of block to parse */
typedef enum BLOCK
{
	REGBLOCK,
	THREAD,
	FUNCBODY
} block_t;


/**
 *	Parse source code to generate syntax structure
 *
 *	@param	prs			Parser structure
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int parse(parser *const prs);


/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
void parser_error(parser *const prs, const error_t num, ...);


/**
 *	Consume the current 'peek token' and lex the next one
 *
 *	@param	prs			Parser structure
 */
void token_consume(parser *const prs);

/**
 *	Try to consume the current 'peek token' and lex the next one
 *
 *	@param	prs			Parser structure
 *	@param	expected	Expected token to consume
 *
 *	@return	@c 1 on consuming 'peek token', @c 0 on otherwise
 */
int token_try_consume(parser *const prs, const token_t expected);

/**
 *	Try to consume the current 'peek token' and lex the next one
 *	If 'peek token' is expected, parser will consume it, otherwise an error will be emitted
 *
 *	@param	prs			Parser structure
 *	@param	expected	Expected token to consume
 *	@param	err			Error to emit
 */
void token_expect_and_consume(parser *const prs, const token_t expected, const error_t err);

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	prs			Parser structure
 *	@param	tokens		Set of specified tokens
 */
void token_skip_until(parser *const prs, const uint8_t tokens);


/**
 *	Parse expression [C99 6.5.17]
 *
 *	expression:
 *		assignment-expression
 *		expression ',' assignment-expression
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_expression(parser *const prs);

/**
 *	Parse assignment expression [C99 6.5.16]
 *
 *	assignment-expression:
 *		conditional-expression
 *		unary-expression assignment-operator assignment-expression
 *
 *	assignment-operator: one of
 *		=  *=  /=  %=  +=  -=  <<=  >>=  &=  ˆ=  |=
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_assignment_expression(parser *const prs);

/**
 *	Parse expression in parentheses
 *
 *	parenthesized-expression:
 *		'(' expression ')'
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_parenthesized_expression(parser *const prs);

/**
 *	Parse constant expression [C99 6.6]
 *
 *	constant-expression:
 *		conditional-expression
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_constant_expression(parser *const prs);

/**
 *	Parse condition
 *	@note	must be evaluated to a simple value
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_condition(parser *const prs);

/**
 *	Parse string literal [C99 6.5.1]
 *
 *	primary-expression:
 *		string-literal
 *
 *	@param	prs			Parser structure
 */
void parse_string_literal(parser *const prs);

/**
 *	Insert @c WIDEN node
 *
 *	@param	prs			Parser structure
 */
void parse_insert_widen(parser *const prs);


/**
 *	Parse a declaration [C99 6.7]
 *	@note Parses a full declaration, which consists of declaration-specifiers,
 *	some number of declarators, and a semicolon
 *
 *	@param	prs			Parser structure
 */
void parse_declaration_inner(parser *const prs);

/**
 *	Parse a top level declaration [C99 6.7]
 *	@note Parses a full declaration, which consists either of declaration-specifiers,
 *	some number of declarators, and a semicolon, or function definition
 *
 *	@param	prs			Parser structure
 */
void parse_declaration_external(parser *const prs);

/**
 *	Parse initializer [C99 6.7.8]
 *
 *	initializer:
 *		assignment-expression
 *		'{' initializer-list '}'
 *
 *	@param	prs			Parser structure
 *	@param	type		Type of variable in declaration
 */
void parse_initializer(parser *const prs, const item_t type);


/**
 *	Parse statement [C99 6.8]
 *
 *	statement:
 *		labeled-statement
 *		compound-statement
 *		expression-statement
 *		selection-statement
 *		iteration-statement
 *		jump-statement
 *
 *	@param	prs			Parser structure
 */
void parse_statement(parser *const prs);

/**
 *	Parse '{}' block [C99 6.8.2]
 *
 *	compound-statement:
 *  	{ block-item-list[opt] }
 *
 *	block-item-list:
 *		block-item
 *		block-item-list block-item
 *
 *	block-item:
 *		declaration
 *		statement
 *
 *	@param	prs			Parser structure
 */
void parse_statement_compound(parser *const prs, const block_t type);


/**
 *	Check if mode is function
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_function(syntax *const sx, const item_t mode);

/**
 *	Check if mode is array
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_array(syntax *const sx, const item_t mode);

/**
 *	Check if mode is string
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_string(syntax *const sx, const item_t mode);

/**
 *	Check if mode is pointer
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_pointer(syntax *const sx, const item_t mode);

/**
 *	Check if mode is struct
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_struct(syntax *const sx, const item_t mode);

/**
 *	Check if mode is floating point
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_float(const item_t mode);

/**
 *	Check if mode is integer
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_int(const item_t mode);

/**
 *	Check if mode is void
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_void(const item_t mode);

/**
 *	Check if mode is undefined
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int mode_is_undefined(const item_t mode);


/**
 *	Add new item to identifiers table
 *
 *	@param	prs			Parser structure
 *	@param	repr		New identifier index in representations table
 *	@param	type		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	mode		New identifier mode
 *
 *	@return	Index of the last item in identifiers table
 */
size_t to_identab(parser *const prs, const size_t repr, const item_t type, const item_t mode);

/**
 *	Add a new record to modes table
 *
 *	@param	prs			Parser structure
 *	@param	mode		@c mode_pointer or @c mode_array
 *	@param	element		Type of element
 *
 *	@return	Index of the new record in modes table, @c SIZE_MAX on failure
 */
item_t to_modetab(parser *const prs, const item_t mode, const item_t element);

#ifdef __cplusplus
} /* extern "C" */
#endif
