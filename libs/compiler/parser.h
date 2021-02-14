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

/**
 *	Parse source code to generate syntax structure
 *
 *	@param	parser		Parser structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int parse(parser *const parser);


/**
 *	Emit an error from parser
 *
 *	@param	parser		Parser structure
 *	@param	num			Error code
 */
void parser_error(parser *const parser, const int num, ...);

/**
 *	Consume the current 'peek token' and lex the next one
 *
 *	@param	parser		Parser structure
 */
void consume_token(parser *const parser);

/**
 *	Try consume the current 'peek token' and lex the next one
 *
 *	@param	parser		Parser structure
 *	@param	expected	Expected token to consume
 *
 *	@return	@c 0, if 'peek token' is expected and consumed, @c 0 otherwise
 */
int try_consume_token(parser *const parser, const token_t expected);

/**
 *	Try consume the current 'peek token' and lex the next one
 *	If that 'peek token' is expected, parser consumes it, otherwise an error is emited
 *
 *	@param	parser		Parser structure
 *	@param	expected	Expected token to consume
 *	@param	err			Error to emit
 */
void expect_and_consume_token(parser *const parser, const token_t expected, const enum ERROR err);

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	parser		Parser structure
 *	@param	tokens		Set of specified tokens
 */
void skip_until(parser *const parser, const unsigned int tokens);


/**
 *	Parse string literal expression [C99 6.5.1]
 *
 *	primary-expression:
 *		string-literal
 *
 *	@param	parser		Parser structure
 */
void parse_string_literal_expression(parser *const parser);

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
 *	@param	parser		Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_assignment_expression(parser *const parser);

/**
 *	Parse expression [C99 6.5.17]
 *
 *	expression:
 *		assignment-expression
 *		expression ',' assignment-expression
 *
 *	@param	parser		Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_expression(parser *const parser);

/**
 *	Parse condition
 *	@note	must be evaluated to a simple value
 *
 *	@param	parser		Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_condition(parser *const parser);

/**
 *	Parse expression in parenthesis
 *
 *	parenthesized-expression:
 *		'(' expression ')'
 *
 *	@param	parser		Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_parenthesized_expression(parser *const parser);

/**
 *	Parse constant expression [C99 6.6]
 *
 *	constant-expression:
 *		conditional-expression
 *
 *	@param	parser		Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_constant_expression(parser *const parser);

/**
 *	Insert @c WIDEN node
 *
 *	@param	parser		Parser structure
 */
void insert_widen(parser *const parser);


/**
 *	Parse a declaration [C99 6.7]
 *	@note Parses a full declaration, which consists of declaration-specifiers,
 *	some number of declarators, and a semicolon
 *
 *	@param	parser		Parser structure
 */
void parse_inner_declaration(parser *const parser);

/**
 *	Parse a top level declaration [C99 6.7]
 *	@note Parses a full declaration, which consists either of declaration-specifiers,
 *	some number of declarators, and a semicolon, or function definition
 *
 *	@param	parser		Parser structure
 */
void parse_external_declaration(parser *const parser);

/**
 *	Parse initializer [C99 6.7.8]
 *
 *	initializer:
 *		assignment-expression
 *		'{' initializer-list '}'
 *
 *	@param	parser		Parser structure
 *	@param	type		Type of variable in declaration
 */
void parse_initializer(parser *const parser, const item_t type);


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
 *	@param	parser		Parser structure
 */
void parse_statement(parser *const context);

/**	@enum The kind of block to parse */
typedef enum { REGBLOCK, THREAD, FUNCBODY } block_type;

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
 *	@param	parser		Parser structure
 */
void parse_compound_statement(parser *const context, const block_type type);


/**
 *	Check if mode is function
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on function, @c 0 otherwise
 */
int is_function(syntax *const sx, const item_t mode);

/**
 *	Check if mode is array
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on array, @c 0 otherwise
 */
int is_array(syntax *const sx, const item_t mode);

/**
 *	Check if mode is string
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on string, @c 0 otherwise
 */
int is_string(syntax *const sx, const item_t mode);

/**
 *	Check if mode is pointer
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on pointer, @c 0 otherwise
 */
int is_pointer(syntax *const sx, const item_t mode);

/**
 *	Check if mode is struct
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on struct, @c 0 otherwise
 */
int is_struct(syntax *const sx, const item_t mode);

/**
 *	Check if mode is floating point
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on floating point, @c 0 otherwise
 */
int is_float(const item_t mode);

/**
 *	Check if mode is integer
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on integer, @c 0 otherwise
 */
int is_int(const item_t mode);

/**
 *	Check if mode is void
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on void, @c 0 otherwise
 */
int is_void(const item_t mode);

/**
 *	Check if mode is undefined
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Mode for check
 *
 *	@return	@c 1 on undefined, @c 0 otherwise
 */
int is_undefined(const item_t mode);

/**
 *	Add new item to identifiers table
 *
 *	@param	parser		Parser structure
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
size_t to_identab(parser *const parser, const size_t repr, const item_t type, const item_t mode);

/**
 *	Add a new record to modes table
 *
 *	@param	parser		Parser structure
 *	@param	mode		@c mode_pointer or @c mode_array
 *	@param	element		Type of element
 *
 *	@return	Index of the new record in modes table, @c SIZE_MAX on failure
 */
item_t to_modetab(parser *const parser, const item_t mode, const item_t element);

#ifdef __cplusplus
} /* extern "C" */
#endif
