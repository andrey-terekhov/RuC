/*
 *	Copyright 2015 Andrey Terekhov
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

#include "codegen.h"
#include <stdlib.h>
#include "defs.h"
#include "errors.h"
#include "tree.h"
#include "uniprinter.h"


typedef struct ad
{
	size_t adcont;
	size_t adbreak;
	size_t adcase;
} ad;


void Declid_gen(syntax *const sx);
void compstmt_gen(syntax *const sx, ad *const context);


void tocode(syntax *const sx, int c)
{
	// printf("tocode tc=%zi pc=%zi) %i\n", sx->tc,
	// mem_get_size(sx), c);
	mem_add(sx, c);
}

void adbreakend(syntax *const sx, ad *const context)
{
	while (context->adbreak)
	{
		const size_t r = mem_get(sx, context->adbreak);
		mem_set(sx, context->adbreak, (int)mem_get_size(sx));
		context->adbreak = r;
	}
}

void adcontbeg(syntax *const sx, ad *const context, size_t ad)
{
	while (context->adcont != ad)
	{
		const size_t r = mem_get(sx, context->adcont);
		mem_set(sx, context->adcont, (int)ad);
		context->adcont = r;
	}
}

void adcontend(syntax *const sx, ad *const context)
{
	while (context->adcont != 0)
	{
		const size_t r = mem_get(sx, context->adcont);
		mem_set(sx, context->adcont, (int)mem_get_size(sx));
		context->adcont = r;
	}
}

void finalop(syntax *const sx)
{
	int c;

	while ((c = node_get_type(tree_get_node(sx))) > 9000)
	{
		if (c != NOP)
		{
			if (c == ADLOGOR)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BNE0);
				stack_push(sx, (int)mem_get_size(sx));
				mem_increase(sx, 1);
			}
			else if (c == ADLOGAND)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BE0);
				stack_push(sx, (int)mem_get_size(sx));
				mem_increase(sx, 1);
			}
			else
			{
				tocode(sx, c);
				if (c == LOGOR || c == LOGAND)
				{
					mem_set(sx, stack_pop(sx), (int)mem_get_size(sx));
				}
				else if (c == COPY00 || c == COPYST)
				{
					tocode(sx, node_get_arg(tree_get_node(sx), 0)); // d1
					tocode(sx, node_get_arg(tree_get_node(sx), 1)); // d2
					tocode(sx, node_get_arg(tree_get_node(sx), 2)); // длина
				}
				else if (c == COPY01 || c == COPY10 || c == COPY0ST || c == COPY0STASS)
				{
					tocode(sx, node_get_arg(tree_get_node(sx), 0)); // d1
					tocode(sx, node_get_arg(tree_get_node(sx), 1)); // длина
				}
				else if (c == COPY11 || c == COPY1ST || c == COPY1STASS)
				{
					tocode(sx, node_get_arg(tree_get_node(sx), 0)); // длина
				}
				else if ((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
						 (c >= ASSR && c <= DIVASSR) || (c >= ASSRV && c <= DIVASSRV) || (c >= POSTINC && c <= DEC) ||
						 (c >= POSTINCV && c <= DECV) || (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
				{
					tocode(sx,node_get_arg(tree_get_node(sx), 0));
				}
			}
		}
		tree_next_node(sx);
	}
}

int Expr_gen(syntax *const sx, int incond)
{
	int wasstring = 0;
	int op;

	while (node_get_type(tree_get_node(sx)) != TExprend)
	{
		switch (op = node_get_type(tree_get_node(sx)))
		{
			case TIdent:
			{
				tree_next_node(sx);
				break;
			}
			case TIdenttoaddr:
			{
				tocode(sx, LA);
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tree_next_node(sx);
				break;
			}
			case TIdenttoval:
			{
				tocode(sx, LOAD);
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tree_next_node(sx);
				break;
			}
			case TIdenttovald:
			{
				tocode(sx, LOADD);
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tree_next_node(sx);
				break;
			}
			case TAddrtoval:
			{
				tocode(sx, LAT);
				tree_next_node(sx);
				break;
			}
			case TAddrtovald:
			{
				tocode(sx, LATD);
				tree_next_node(sx);
				break;
			}
			case TConst:
			{
				tocode(sx, LI);
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tree_next_node(sx);
				break;
			}
			case TConstd:
			{
				tocode(sx, LID);
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tocode(sx, node_get_arg(tree_get_node(sx), 1));
				tree_next_node(sx);
				break;
			}
			case TString:
			case TStringd:
			{
				int n = node_get_arg(tree_get_node(sx), 0);

				tocode(sx, LI);
				size_t res = mem_get_size(sx) + 4;
				tocode(sx, (int)res);
				tocode(sx, B);
				mem_increase(sx, 2);
				for (int i = 0; i < n; i++)
				{
					if (op == TString)
					{
						tocode(sx, node_get_arg(tree_get_node(sx), i + 1));
					}
					else
					{
						tocode(sx, node_get_arg(tree_get_node(sx), 2 * i + 1));
						tocode(sx, node_get_arg(tree_get_node(sx), 2 * i + 1 + 1));
					}
				}
				mem_set(sx, res - 1, n);
				mem_set(sx, res - 2, (int)mem_get_size(sx));
				wasstring = 1;
				tree_next_node(sx);
				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				tree_next_node(sx); // TExpend
				break;
			}
			case TBeginit:
			{
				int n = node_get_arg(tree_get_node(sx), 0);
				int i;

				tocode(sx, BEGINIT);
				tocode(sx, n);
				tree_next_node(sx);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
					tree_next_node(sx); // TExpend
				}
				break;
			}
			case TStructinit:
			{
				int n = node_get_arg(tree_get_node(sx), 0);
				int i;

				tree_next_node(sx);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
					tree_next_node(sx); // TExprend
				}
				break;
			}
			case TSliceident:
			{
				tocode(sx, LOAD); // параметры - смещение идента и тип элемента
				tocode(sx, node_get_arg(tree_get_node(sx), 0)); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				int eltype = node_get_arg(tree_get_node(sx), (node_get_type(tree_get_node(sx)) == TSlice) ? 0 : 1);

				tree_next_node(sx);
				Expr_gen(sx, 0);
				tree_next_node(sx); // TExprend
				tocode(sx, SLICE);
				tocode(sx, size_of(sx, eltype));
				if (eltype > 0 && mode_get(sx, eltype) == MARRAY)
				{
					tocode(sx, LAT);
				}
				break;
			}
			case TSelect:
			{
				tocode(sx, SELECT); // SELECT field_displ
				tocode(sx, node_get_arg(tree_get_node(sx), 0));
				tree_next_node(sx);
				break;
			}
			case TPrint:
			{
				tocode(sx, PRINT);
				tocode(sx, node_get_arg(tree_get_node(sx), 0)); // type
				tree_next_node(sx);
				break;
			}
			case TCall1:
			{
				int i;
				int n = node_get_arg(tree_get_node(sx), 0);

				tocode(sx, CALL1);
				tree_next_node(sx);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
					tree_next_node(sx); // TExprend
				}
				break;
			}
			case TCall2:
			{
				tocode(sx, CALL2);

				tocode(sx, ident_get_displ(sx, node_get_arg(tree_get_node(sx), 0)));
				tree_next_node(sx);
				break;
			}
			default:
			{
				break;
			}
		}

		finalop(sx);

		if (node_get_type(tree_get_node(sx)) == TCondexpr)
		{
			if (incond)
			{
				return wasstring;
			}
			else
			{
				size_t ad = 0;
				do
				{
					tocode(sx, BE0);
					size_t adelse = mem_get_size(sx);
					mem_increase(sx, 1);
					tree_next_node(sx);
					Expr_gen(sx, 0); // then
					tree_next_node(sx); // TExprend
					tocode(sx, B);
					mem_add(sx, (int)ad);
					ad = mem_get_size(sx) - 1;
					mem_set(sx, adelse, (int)mem_get_size(sx));
					Expr_gen(sx, 1); // else или cond
				} while (node_get_type(tree_get_node(sx)) == TCondexpr);

				while (ad)
				{
					int r = mem_get(sx, ad);
					mem_set(sx, ad, (int)mem_get_size(sx));
					ad = r;
				}
			}

			finalop(sx);
		}
	}
	//tree_next_node(sx);
	return wasstring;
}

void Stmt_gen(syntax *const sx, ad *const context)
{
	switch (node_get_type(tree_get_node(sx)))
	{
		case NOP:
		{
			tree_next_node(sx);
			break;
		}
		case CREATEDIRECTC:
		{
			tocode(sx, CREATEDIRECTC);
			tree_next_node(sx);
			break;
		}
		case EXITDIRECTC:
		case EXITC:
		{
			tocode(sx, EXITC);
			tree_next_node(sx);
			break;
		}
		case TStructbeg:
		{
			tocode(sx, B);
			tocode(sx, 0);
			proc_set(sx, node_get_arg(tree_get_node(sx), 0), (int)mem_get_size(sx));
			tree_next_node(sx);
			break;
		}
		case TStructend:
		{
			int numproc = node_get_arg(tree_get_node(sx), 0);

			tocode(sx, STOP);
			mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));	
			tree_next_node(sx);
			break;
		}
		case TBegin:
		{
			tree_next_node(sx); // TBegin
			compstmt_gen(sx, context);
			tree_next_node(sx); // TEnd
			break;
		}
		case TIf:
		{
			int ref_else = node_get_arg(tree_get_node(sx), 0);

			tree_next_node(sx);
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, BE0);
			size_t ad = mem_get_size(sx);
			mem_increase(sx, 1);
			Stmt_gen(sx, context);
			if (ref_else)
			{
				mem_set(sx, ad, (int)mem_get_size(sx) + 2);
				tocode(sx, B);
				ad = mem_get_size(sx);
				mem_increase(sx, 1);
				Stmt_gen(sx, context);
			}
			mem_set(sx, ad, (int)mem_get_size(sx));
			break;
		}
		case TWhile:
		{
			size_t oldbreak = context->adbreak;
			size_t oldcont = context->adcont;
			size_t ad = mem_get_size(sx);

			tree_next_node(sx);
			context->adcont = ad;
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, BE0);
			context->adbreak = mem_get_size(sx);
			mem_add(sx, 0);	
			Stmt_gen(sx, context);
			adcontbeg(sx, context, ad);
			tocode(sx, B);
			tocode(sx, (int)ad);
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TDo:
		{
			size_t oldbreak = context->adbreak;
			size_t oldcont = context->adcont;
			size_t ad = mem_get_size(sx);

			tree_next_node(sx);

			context->adcont = 0;
			context->adbreak = 0;

			Stmt_gen(sx, context);
			adcontend(sx, context);
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, BNE0);
			tocode(sx, (int)ad);
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TFor:
		{
			node *tfor = tree_get_node(sx);
			int ref_from = node_get_arg(tfor, 0);
			int ref_cond = node_get_arg(tfor, 1);
			int ref_incr = node_get_arg(tfor, 2);

			size_t oldbreak = context->adbreak;
			size_t oldcont = context->adcont;

			node temp = node_get_child(tfor, 0);
			tree_set_node(sx, &temp);
			size_t child_stmt = 0;

			if (ref_from)
			{
				Expr_gen(sx, 0); // init
				tree_next_node(sx); // TExprend
				child_stmt++;
			}

			size_t initad = mem_get_size(sx);
			context->adcont = 0;
			context->adbreak = 0;

			if (ref_cond)
			{
				Expr_gen(sx, 0); // cond
				tree_next_node(sx); // TExprend
				tocode(sx, BE0);
				context->adbreak = mem_get_size(sx);
				mem_add(sx, 0);
				child_stmt++;
			}

			if (ref_incr)
			{
				child_stmt++;
			}

			temp = node_get_child(tfor, child_stmt);

			Stmt_gen(sx, context); // ???? был 0
			adcontend(sx, context);

			if (ref_incr)
			{
				node incr = node_get_child(tfor, child_stmt - 1);
				tree_set_node(sx, &incr);
				Expr_gen(sx, 0); // incr
				tree_next_node(sx); // TExprend
			}

			*tfor = temp;
			tree_set_node(sx, tfor);
			
			tocode(sx, B);
			tocode(sx, (int)initad);
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TGoto:
		{
			int id1 = node_get_arg(tree_get_node(sx), 0);
			int id = id1 > 0 ? id1 : -id1;

			tocode(sx, B);
			int a = ident_get_displ(sx, id);

			if (a > 0) // метка уже описана
			{
				tocode(sx, a);
			}
			else // метка еще не описана
			{
				ident_set_displ(sx, id, -(int)mem_get_size(sx));
				tocode(sx, id1 < 0 ? 0 : a);	// первый раз встретился переход на еще
												// не описанную метку или нет
			}

			tree_next_node(sx);
			break;
		}
		case TLabel:
		{
			int id = node_get_arg(tree_get_node(sx), 0);
			int a = ident_get_displ(sx, id);

			if (a < 0) // были переходы на метку
			{
				while (a) // проставить ссылку на метку во всех ранних переходах
				{
					int r = mem_get(sx, -a);
					mem_set(sx, -a, (int)mem_get_size(sx));
					a = r;
				}
			}
			ident_set_displ(sx, id, (int)mem_get_size(sx));
			tree_next_node(sx);
			break;
		}
		case TSwitch:
		{
			size_t oldbreak = context->adbreak;
			size_t oldcase = context->adcase;

			context->adbreak = 0;
			context->adcase = 0;

			tree_next_node(sx);
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			Stmt_gen(sx, context);
			if (context->adcase > 0)
			{
				mem_set(sx, context->adcase, (int)mem_get_size(sx));
			}
			context->adcase = oldcase;
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			break;
		}
		case TCase:
		{
			if (context->adcase)
			{
				mem_set(sx, context->adcase, (int)mem_get_size(sx));
			}
			tocode(sx, _DOUBLE);

			tree_next_node(sx);
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, EQEQ);
			tocode(sx, BE0);
			context->adcase = mem_get_size(sx);
			mem_increase(sx, 1);
			Stmt_gen(sx, context);
			break;
		}
		case TDefault:
		{
			if (context->adcase)
			{
				mem_set(sx, context->adcase, (int)mem_get_size(sx));
			}
			context->adcase = 0;

			tree_next_node(sx);
			Stmt_gen(sx, context);
			break;
		}
		case TBreak:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adbreak);
			context->adbreak = mem_get_size(sx) - 1;

			tree_next_node(sx);
			break;
		}
		case TContinue:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adcont);
			context->adcont = mem_get_size(sx) - 1;
			tree_next_node(sx);
			break;
		}
		case TReturnvoid:
		{
			tocode(sx, RETURNVOID);
			tree_next_node(sx);
			break;
		}
		case TReturnval:
		{
			int d = node_get_arg(tree_get_node(sx), 0);
			tree_next_node(sx);

			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, RETURNVAL);
			tocode(sx, d);
			break;
		}
		case TPrintid:
		{
			tocode(sx, PRINTID);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // ссылка в identtab
			tree_next_node(sx);
			break;
		}
		case TPrintf:
		{
			tocode(sx, PRINTF);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // общий размер того,
														   // что надо вывести
			tree_next_node(sx);
			break;
		}
		case TGetid:
		{
			tocode(sx, GETID);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // ссылка в identtab
			tree_next_node(sx);
			break;
		}
		case SETMOTOR:
		{
			tree_next_node(sx);
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			tocode(sx, SETMOTORC);
			break;
		}
		default:
		{
			Expr_gen(sx, 0);
			tree_next_node(sx); // TExprend
			break;
		}
	}
}

void Struct_init_gen(syntax *const sx)
{
	if (node_get_type(tree_get_node(sx)) == TStructinit)
	{
		const int n = node_get_arg(tree_get_node(sx), 0);
		tree_next_node(sx);

		for (int i = 0; i < n; i++)
		{
			Struct_init_gen(sx);
			tree_next_node(sx); // TExprend
		}
	}
	else
	{
		Expr_gen(sx, 0);
	}
}

void Declid_gen(syntax *const sx)
{
	int olddispl = node_get_arg(tree_get_node(sx), 0);
	int telem = node_get_arg(tree_get_node(sx), 1);
	int N = node_get_arg(tree_get_node(sx), 2);
	int element_len = size_of(sx, telem);
	int all = node_get_arg(tree_get_node(sx), 3);
	int iniproc = node_get_arg(tree_get_node(sx), 4);
	int usual = node_get_arg(tree_get_node(sx), 5);
	int instruct = node_get_arg(tree_get_node(sx), 6);
	// all - общее кол-во слов в структуре
	// для массивов есть еще usual // == 0 с пустыми границами,
	// == 1 без пустых границ,
	// all == 0 нет инициализатора,
	// all == 1 есть инициализатор
	// all == 2 есть инициализатор только из строк

	if (N == 0) // обычная переменная int a; или struct point p;
	{
		if (iniproc)
		{
			tocode(sx, STRUCTWITHARR);
			tocode(sx, olddispl);
			tocode(sx, proc_get(sx, iniproc));
		}
		if (all) // int a = или struct{} a =
		{
			if (telem > 0 && mode_get(sx, telem) == MSTRUCT)
			{
				tree_next_node(sx);
				Struct_init_gen(sx);

				tocode(sx, COPY0STASS);
				tocode(sx, olddispl);
				tocode(sx, all); // общее кол-во слов
			}
			else
			{
				tree_next_node(sx);
				Expr_gen(sx, 0);

				tocode(sx, telem == LFLOAT ? ASSRV : ASSV);
				tocode(sx, olddispl);
			}
		}
	}
	else // Обработка массива int a[N1]...[NN] =
	{
		tocode(sx, DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN
								 // уже лежат на стеке
		tocode(sx, all == 0 ? N : abs(N) - 1);
		tocode(sx, element_len);
		tocode(sx, olddispl);
		tocode(sx, proc_get(sx, iniproc));
		tocode(sx, usual);
		tocode(sx, all);
		tocode(sx, instruct);

		if (all) // all == 1, если есть инициализация массива
		{
			tree_next_node(sx);
			Expr_gen(sx, 0);

			tocode(sx, ARRINIT); // ARRINIT N d all displ usual
			tocode(sx, abs(N));
			tocode(sx, element_len);
			tocode(sx, olddispl);
			tocode(sx, usual); // == 0 с пустыми границами
									// == 1 без пустых границ и без иниц
		}
	}
}

void compstmt_gen(syntax *const sx, ad *const context)
{
	while (node_get_type(tree_get_node(sx)) != TEnd)
	{
		switch (node_get_type(tree_get_node(sx)))
		{
			case TDeclarr:
			{
				int N = node_get_arg(tree_get_node(sx), 0);

				tree_next_node(sx);

				for (int i = 0; i < N; i++)
				{
					Expr_gen(sx, 0);
					tree_next_node(sx); // TExprend
				}
				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				tree_next_node(sx); // TExpend
				break;
			}
			default:
			{
				Stmt_gen(sx, context);
				break;
			}
		}
	}
	//tree_next_node(sx); // TEnd
}

/** Генерация кодов */
int codegen(syntax *const sx)
{
	int is_end = 0;
	node root = node_get_root(sx);
	tree_set_node(sx, &root);

	ad context;

	tree_next_node(sx);

	while (!is_end)
	{
		switch (node_get_type(tree_get_node(sx)))
		{
			case TEnd:
				is_end = tree_next_node(sx);
				break;
			case TFuncdef:
			{
				int identref = node_get_arg(tree_get_node(sx), 0);
				int maxdispl = node_get_arg(tree_get_node(sx), 1);
				int fn = ident_get_displ(sx, identref);

				func_set(sx, fn, mem_get_size(sx));
				tocode(sx, FUNCBEG);
				tocode(sx, maxdispl);
				size_t old_pc = mem_get_size(sx);
				mem_increase(sx, 1);
				tree_next_node(sx);
				tree_next_node(sx); // TBegin
				compstmt_gen(sx, &context);
				tree_next_node(sx); // TEnd
				mem_set(sx, old_pc, (int)mem_get_size(sx));
				break;
			}
			case TDeclarr:
			{
				int N = node_get_arg(tree_get_node(sx), 0);

				tree_next_node(sx);

				for (int i = 0; i < N; i++)
				{
					Expr_gen(sx, 0);
					tree_next_node(sx); // TExprend
				}
				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				tree_next_node(sx); // TExpend
				break;
			}
			case NOP:
			{
				tree_next_node(sx);
				break;
			}
			case TStructbeg:
			{
				tocode(sx, B);
				tocode(sx, 0);
				proc_set(sx, node_get_arg(tree_get_node(sx), 0), (int)mem_get_size(sx));
				tree_next_node(sx);
				break;
			}
			case TStructend:
			{
				int numproc = node_get_arg(tree_get_node(sx), 0);

				tocode(sx, STOP);
				mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));
				tree_next_node(sx);
				break;
			}
			default:
			{
				printf("tc=%zi tree[tc-2]=%i tree[tc-1]=%i\n", sx->tc, sx->tree[sx->tc - 2],
					   sx->tree[sx->tc - 1]);
				break;
			}
		}
	}
	tocode(sx, CALL1);
	tocode(sx, CALL2);
	tocode(sx, ident_get_displ(sx, sx->main_ref));
	tocode(sx, STOP);

	return 0;
}

/** Вывод таблиц в файл */
void output_export(universal_io *const io, const syntax *const sx)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %i %zi\n", mem_get_size(sx), sx->funcnum, sx->id,
				   sx->rp, sx->md, sx->maxdisplg, sx->main_ref);

	for (size_t i = 0; i < mem_get_size(sx); i++)
	{
		uni_printf(io, "%i ", mem_get(sx, i));
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->funcnum; i++)
	{
		uni_printf(io, "%zi ", func_get(sx, i));
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->id; i++)
	{
		uni_printf(io, "%i ", sx->identab[i]);
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->rp; i++)
	{
		uni_printf(io, "%i ", sx->reprtab[i]);
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->md; i++)
	{
		uni_printf(io, "%i ", mode_get(sx, i));
	}
	uni_printf(io, "\n");
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_vm(universal_io *const io, syntax *const sx)
{
	if (!out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	int ret = codegen(sx);
	if (!ret)
	{
		output_export(io, sx);
	}

	return ret;
}