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
 *	Emit an error from parser
 *
 *	@param	context	Parser structure
 *	@param	err		Error code
 */
void parser_error(analyzer *const context, const enum ERROR err);

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	context		Parser structure
 *	@param	tokens		Set of the specified tokens
 */
void skip_until(analyzer *const context, const unsigned int tokens);

int scanner(analyzer *context);
int newdecl(syntax *const sx, const int type, const int element_type);
int evaluate_params(analyzer *context, int num, char32_t formatstr[], int formattypes[], char32_t placeholders[]);
int is_function(syntax *const sx, const int t);
int is_array(syntax *const sx, const int t);
int is_string(syntax *const sx, const int t);
int is_pointer(syntax *const sx, const int t);
int is_struct(syntax *const sx, const int t);
int is_float(const int t);
int is_int(const int t);
int szof(analyzer *context, int type);
void mustbe(analyzer *context, int what, int e);
void mustbe_complex(analyzer *context, int what, int e);
void totree(analyzer *context, int op);
void totreef(analyzer *context, int op);
int toidentab(analyzer *context, int f, int type);
void binop(analyzer *context, int sp);
void toval(analyzer *context);
void insertwiden(analyzer *context);
void applid(analyzer *context);
void actstring(int type, analyzer *context);
void mustbestring(analyzer *context);
void mustbepointstring(analyzer *context);

void mustberow(analyzer *context);

void mustbeint(analyzer *context);

void mustberowofint(analyzer *context);

void mustberowoffloat(analyzer *context);

void primaryexpr(analyzer *context);

void index_check(analyzer *context);

int find_field(analyzer *context, int stype);

void selectend(analyzer *context);

void postexpr(analyzer *context);

void unarexpr(analyzer *context);

void exprinbrkts(analyzer *context, int er);

void exprassninbrkts(analyzer *context, int er);

int prio(int op);

void subexpr(analyzer *context);

int intopassn(int next);

int opassn(analyzer *context);

void condexpr(analyzer *context);

void inition(analyzer *context, int decl_type);

void struct_init(analyzer *context, int decl_type);

void exprassnvoid(analyzer *context);

void exprassn(analyzer *context, int level);

void expr(analyzer *context, int level);

void exprval(analyzer *context);

void exprassnval(analyzer *context);

void array_init(analyzer *context, int decl_type);

int arrdef(analyzer *context, int t);

void decl_id(analyzer *context, int decl_type);


int idorpnt(analyzer *context, int e, int t);

int struct_decl_list(analyzer *context);

int gettype(analyzer *context);

void function_definition(analyzer *context);

int func_declarator(analyzer *context, int level, int func_d, int firstdecl);

void ext_decl(analyzer *context);


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
 */
void parse_statement(analyzer *const context);

typedef enum block_type
{ REGBLOCK = 1, THREAD = 2, SWITCH = -1, FUNCBODY = 0 } block_type;

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
 */
void parse_compound_statement(analyzer *const context, const block_type type);

/// ParseTopLevelDecl - Parse one top-level declaration, return whatever the
/// action tells us to.  This returns true if the EOF was encountered.
///
///   top-level-declaration:
///           declaration
void parse_top_level_declaration(analyzer *const context);

#ifdef __cplusplus
} /* extern "C" */
#endif
