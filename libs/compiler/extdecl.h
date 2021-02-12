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

#include "analyzer.h"


#ifdef __cplusplus
extern "C" {
#endif


int scanner(parser *context);

item_t newdecl(syntax *const sx, const item_t type, const item_t element_type);

int double_to_tree(vector *const tree, const double num);

double double_from_tree(vector *const tree);

void context_error(parser *const context, const int num);

int evaluate_params(parser *context, int num, char32_t formatstr[], int formattypes[], char32_t placeholders[]);

int is_function(syntax *const sx, const int t);

int is_array(syntax *const sx, const int t);

int is_string(syntax *const sx, const int t);

int is_pointer(syntax *const sx, const int t);

int is_struct(syntax *const sx, const int t);

int is_float(const int t);

int is_int(const int t);

int szof(parser *context, int type);

void mustbe(parser *context, int what, int e);

void mustbe_complex(parser *context, int what, int e);

void totree(parser *context, item_t op);

void totreef(parser *context, item_t op);

int toidentab(parser *context, int f, int type);

void binop(parser *context, int sp);

void toval(parser *context);

void insertwiden(parser *context);

void applid(parser *context);

void actstring(int type, parser *context);

void mustbestring(parser *context);

void mustbepointstring(parser *context);

void mustberow(parser *context);

void mustbeint(parser *context);

void mustberowoffloat(parser *context);

void primaryexpr(parser *context);

void index_check(parser *context);

int find_field(parser *context, int stype);

void selectend(parser *context);


void postexpr(parser *context);

void unarexpr(parser *context);

void exprinbrkts(parser *context, int er);

void exprassninbrkts(parser *context, int er);

int prio(int op);

void subexpr(parser *context);

int intopassn(int next);

int opassn(parser *context);

void condexpr(parser *context);

void inition(parser *context, int decl_type);

void struct_init(parser *context, int decl_type);

void exprassnvoid(parser *context);

void exprassn(parser *context, int level);

void expr(parser *context, int level);

void exprval(parser *context);

void exprassnval(parser *context);

void array_init(parser *context, int decl_type);

int arrdef(parser *context, item_t t);

void decl_id(parser *context, int decl_type);

void statement(parser *context);

item_t idorpnt(parser *context, int e, item_t t);

int struct_decl_list(parser *context);

item_t gettype(parser *context);

/** Debug from here */
void block(parser *context, int b);

void function_definition(parser *context);

int func_declarator(parser *context, int level, int func_d, int firstdecl);

void ext_decl(parser *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
