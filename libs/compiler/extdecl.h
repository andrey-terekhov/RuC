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


int scanner(analyzer *context);

item_t newdecl(syntax *const sx, const item_t type, const item_t element_type);

int double_to_tree(vector *const tree, const double num);

double double_from_tree(vector *const tree);

void context_error(analyzer *const context, const int num);

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

void totree(analyzer *context, item_t op);

void totreef(analyzer *context, item_t op);

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

int arrdef(analyzer *context, item_t t);

void decl_id(analyzer *context, int decl_type);

void statement(analyzer *context);

item_t idorpnt(analyzer *context, int e, item_t t);

int struct_decl_list(analyzer *context);

item_t gettype(analyzer *context);

/** Debug from here */
void block(analyzer *context, int b);

void function_definition(analyzer *context);

int func_declarator(analyzer *context, int level, int func_d, int firstdecl);

void ext_decl(analyzer *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
