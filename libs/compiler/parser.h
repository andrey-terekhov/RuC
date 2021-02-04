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
int try_consume_token(parser *const parser, const token expected);

/**
 *	Try consume the current 'peek token' and lex the next one
 *	If that 'peek token' is expected, parser consumes it, otherwise an error is emited
 *
 *	@param	parser		Parser structure
 *	@param	expected	Expected token to consume
 *	@param	err			Error to emit
 */
void expect_and_consume_token(parser *const parser, const token expected, const enum ERROR err);

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
int parse_assignment_expression(parser *const parser);

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
int parse_expression(parser *const parser);

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
int parse_parenthesized_expression(parser *const parser);

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
int parse_constant_expression(parser *const parser);


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
void parse_initializer(parser *const parser, const int type);


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
 *	@param	parser	Parser structure
 */
void parse_compound_statement(parser *const context, const block_type type);




int scanner(parser *context);
int newdecl(syntax *const sx, const int type, const int element_type);
int evaluate_params(parser *context, int num, char32_t formatstr[], int formattypes[], char32_t placeholders[]);
int is_function(syntax *const sx, const int t);
int is_array(syntax *const sx, const int t);
int is_string(syntax *const sx, const int t);
int is_pointer(syntax *const sx, const int t);
int is_struct(syntax *const sx, const int t);
int is_float(const int t);
int is_int(const int t);
int is_void(const int t);
int is_undefined(const int t);
int szof(parser *context, int type);
void mustbe(parser *context, int what, int e);
void totree(parser *context, int op);
void totreef(parser *context, int op);
int toidentab(parser *context, size_t repr, int f, int type);
void binop(parser *context, int sp);
void toval(parser *context);
void insertwiden(parser *context);
void applid(parser *context);
void actstring(int type, parser *context);
void mustbestring(parser *context);
void mustbepointstring(parser *context);
void mustberow(parser *context);
void mustbeint(parser *context);
void mustberowofint(parser *context);
void mustberowoffloat(parser *context);
void primaryexpr(parser *context);
int find_field(parser *context, int stype);
void selectend(parser *context);
void postexpr(parser *context);
void unarexpr(parser *context);
void exprassninbrkts(parser *context, int er);
int prio(int op);
void subexpr(parser *context);
int intopassn(int next);
int opassn(parser *context);
void condexpr(parser *context);
void exprassnvoid(parser *context);
void exprassn(parser *context, int level);
void expr(parser *context, int level);
int exprval(parser *context);
void exprassnval(parser *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
