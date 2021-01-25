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


int local_tc;

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
	// printf("tocode local_tc=%i sx->pc %zi) %i\n", local_tc,
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

	while ((c = sx->tree[local_tc]) > 9000)
	{
		local_tc++;
		if (c != NOP)
		{
			if (c == ADLOGOR)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BNE0);
				sx->tree[sx->tree[local_tc++]] = (int)mem_get_size(sx);
				mem_increase(sx, 1);
			}
			else if (c == ADLOGAND)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BE0);
				sx->tree[sx->tree[local_tc++]] = (int)mem_get_size(sx);
				mem_increase(sx, 1);
			}
			else
			{
				tocode(sx, c);
				if (c == LOGOR || c == LOGAND)
				{
					mem_set(sx, sx->tree[local_tc++], (int)mem_get_size(sx));
				}
				else if (c == COPY00 || c == COPYST)
				{
					tocode(sx, sx->tree[local_tc++]); // d1
					tocode(sx, sx->tree[local_tc++]); // d2
					tocode(sx, sx->tree[local_tc++]); // длина
				}
				else if (c == COPY01 || c == COPY10 || c == COPY0ST || c == COPY0STASS)
				{
					tocode(sx, sx->tree[local_tc++]); // d1
					tocode(sx, sx->tree[local_tc++]); // длина
				}
				else if (c == COPY11 || c == COPY1ST || c == COPY1STASS)
				{
					tocode(sx, sx->tree[local_tc++]); // длина
				}
				else if ((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
						 (c >= ASSR && c <= DIVASSR) || (c >= ASSRV && c <= DIVASSRV) || (c >= POSTINC && c <= DEC) ||
						 (c >= POSTINCV && c <= DECV) || (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
				{
					tocode(sx, sx->tree[local_tc++]);
				}
			}
		}

		tree_next_node(sx);
		printf("%i tc=%i :finalop \n", node_get_type(tree_get_node(sx)), local_tc);

	}
}

int sz_of(syntax *const sx, int type)
{
	return type == LFLOAT ? 2 : (type > 0 && mode_get(sx, type) == MSTRUCT) ? mode_get(sx, type + 1) : 1;
}

int Expr_gen(syntax *const sx, int incond)
{
	int flagprim = 1;
	int eltype;
	int wasstring = 0;
	int op;

	while (flagprim)
	{
		switch (op = sx->tree[local_tc++])
		{
			case TIdent:
			{
				sx->anstdispl = sx->tree[local_tc++];

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TIdent \n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TIdenttoaddr:
			{
				tocode(sx, LA);
				tocode(sx, sx->anstdispl = sx->tree[local_tc++]);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TIdenttoaddr \n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TIdenttoval:
			{
				tocode(sx, LOAD);
				tocode(sx, sx->tree[local_tc++]);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TIdenttoval \n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TIdenttovald:
			{
				tocode(sx, LOADD);
				tocode(sx, sx->tree[local_tc++]);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TIdenttovald\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TAddrtoval:
			{
				tocode(sx, LAT);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TAddrtoval\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TAddrtovald:
			{
				tocode(sx, LATD);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TAddrtovald\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TConst:
			{
				tocode(sx, LI);
				tocode(sx, sx->tree[local_tc++]);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TConst\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TConstd:
			{
				tocode(sx, LID);
				tocode(sx, sx->tree[local_tc++]);
				tocode(sx, sx->tree[local_tc++]);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TConstd\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TString:
			case TStringd:
			{
				int n = node_get_arg(tree_get_node(sx), 0);

				local_tc++;

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
						local_tc++;
					}
					else
					{
						tocode(sx, node_get_arg(tree_get_node(sx), 2 * i + 1));
						tocode(sx, node_get_arg(tree_get_node(sx), 2 * i + 1 + 1));
						local_tc += 2;
					}
				}
				mem_set(sx, res - 1, n);
				mem_set(sx, res - 2, (int)mem_get_size(sx));
				wasstring = 1;

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TString\n", node_get_type(tree_get_node(sx)), local_tc);

				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				break;
			}
			case TBeginit:
			{
				int n = node_get_arg(tree_get_node(sx), 0);
				local_tc++;
				int i;

				tocode(sx, BEGINIT);
				tocode(sx, n);

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TBeginit\n", node_get_type(tree_get_node(sx)), local_tc);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TStructinit:
			{
				int n = node_get_arg(tree_get_node(sx), 0);
				local_tc++;
				int i;

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TStructinit\n", node_get_type(tree_get_node(sx)), local_tc);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TSliceident:
			{
				tocode(sx, LOAD); // параметры - смещение идента и тип элемента
				tocode(sx, node_get_arg(tree_get_node(sx), 0)); // продолжение в след case

				local_tc++;
			}
			case TSlice: // параметр - тип элемента
			{
				eltype = node_get_arg(tree_get_node(sx), (node_get_type(tree_get_node(sx)) == TSlice) ? 0 : 1);

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TSlice\n", node_get_type(tree_get_node(sx)), local_tc);

				Expr_gen(sx, 0);
				tocode(sx, SLICE);
				tocode(sx, sz_of(sx, eltype));
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

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TSelect\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TPrint:
			{
				tocode(sx, PRINT);
				tocode(sx, node_get_arg(tree_get_node(sx), 0)); // type

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i :Expr_gen TPrint\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TCall1:
			{
				int i;
				int n = node_get_arg(tree_get_node(sx), 0);

				local_tc++;

				tocode(sx, CALL1);

				tree_next_node(sx);
				printf("%i tc=%i: Expr_gen TCall1\n", node_get_type(tree_get_node(sx)), local_tc);

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TCall2:
			{
				tocode(sx, CALL2);
				tocode(sx, sx->identab[node_get_arg(tree_get_node(sx), 0) + 3]);

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i: Expr_gen TCall2\n", node_get_type(tree_get_node(sx)), local_tc);

				break;
			}
			default:
			{
				local_tc--;
				break;
			}
		}

		finalop(sx);

		if (node_get_type(tree_get_node(sx)) == TCondexpr)
		{
			if (incond)
			{
				printf("%i tc=%i: Expr_gen TCondexpr\n", node_get_type(tree_get_node(sx)), local_tc);

				return wasstring;
			}
			else
			{
				size_t ad = 0;
				do
				{
					local_tc++;
					tocode(sx, BE0);
					size_t adelse = mem_get_size(sx);
					mem_increase(sx, 1);

					tree_next_node(sx);
					printf("%i tc=%i: Expr_gen TCondexpr\n", node_get_type(tree_get_node(sx)), local_tc);

					Expr_gen(sx, 0); // then
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
		if (node_get_type(tree_get_node(sx)) == TExprend)
		{		
			local_tc++;
			flagprim = 0;

			tree_next_node(sx);
			printf("%i tc=%i: Expr_gen TExprend\n", node_get_type(tree_get_node(sx)), local_tc);
		}
	}
	return wasstring;
}

void Stmt_gen(syntax *const sx, ad *const context)
{
	local_tc++;
	switch (node_get_type(tree_get_node(sx)))
	{
		case NOP:
		{
			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen NOP\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case CREATEDIRECTC:
		{
			tocode(sx, CREATEDIRECTC);

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen CREATEDIRECTC\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case EXITDIRECTC:
		case EXITC:
		{
			tocode(sx, EXITC);

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen EXITC\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TStructbeg:
		{
			tocode(sx, B);
			tocode(sx, 0);
			proc_set(sx, node_get_arg(tree_get_node(sx), 0), (int)mem_get_size(sx));

			local_tc++;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TStructbeg\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TStructend:
		{
			int numproc = sx->tree[node_get_arg(tree_get_node(sx), 0) + 1];

			local_tc++;

			tocode(sx, STOP);
			mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));
						
			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TStructend\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TBegin:
			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TBegin\n", node_get_type(tree_get_node(sx)), local_tc);

			compstmt_gen(sx, context);
			break;

		case TIf:
		{
			int elseref = node_get_arg(tree_get_node(sx), 0);

			local_tc++;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TIf\n", node_get_type(tree_get_node(sx)), local_tc);

			Expr_gen(sx, 0);
			tocode(sx, BE0);
			size_t ad = mem_get_size(sx);
			mem_increase(sx, 1);
			Stmt_gen(sx, context);
			if (elseref)
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
			printf("%i tc=%i: Stmt_gen TWhile\n", node_get_type(tree_get_node(sx)), local_tc);

			context->adcont = ad;
			Expr_gen(sx, 0);
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
			printf("%i tc=%i: Stmt_gen TDo\n", node_get_type(tree_get_node(sx)), local_tc);

			context->adcont = context->adbreak = 0;
			Stmt_gen(sx, context);
			adcontend(sx, context);
			Expr_gen(sx, 0);
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
			int ref_stmt = node_get_arg(tfor, 3);
			local_tc += 4;

			size_t oldbreak = context->adbreak;
			size_t oldcont = context->adcont;

			node temp = node_get_child(tfor, 0);
			tree_set_node(sx, &temp);
			size_t child_stmt = 0;

			if (ref_from)
			{
				Expr_gen(sx, 0); // init
				child_stmt++;
			}

			size_t initad = mem_get_size(sx);
			context->adcont = context->adbreak = 0;

			if (ref_cond)
			{
				Expr_gen(sx, 0); // cond
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

			int incrtc = local_tc;
			local_tc = ref_stmt;

			Stmt_gen(sx, context); // ???? был 0
			adcontend(sx, context);

			if (ref_incr)
			{
				node incr = node_get_child(tfor, child_stmt - 1);
				tree_set_node(sx, &incr);

				int endtc = local_tc;
				local_tc = incrtc;

				Expr_gen(sx, 0); // incr
				local_tc = endtc;	
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
			int a;
			int id = id1 > 0 ? id1 : -id1;

			local_tc++;

			tocode(sx, B);
			if ((a = sx->identab[id + 3]) > 0) // метка уже описана
			{
				tocode(sx, a);
			}
			else // метка еще не описана
			{
				sx->identab[id + 3] = -(int)mem_get_size(sx);
				tocode(sx,
					   id1 < 0 ? 0 : a); // первый раз встретился переход на еще
										 // не описанную метку или нет
			}

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TGoto\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TLabel:
		{
			int id = node_get_arg(tree_get_node(sx), 0);
			int a;

			local_tc++;

			if ((a = sx->identab[id + 3]) < 0) // были переходы на метку
			{
				while (a) // проставить ссылку на метку во всех ранних переходах
				{
					int r = mem_get(sx, -a);
					mem_set(sx, -a, (int)mem_get_size(sx));
					a = r;
				}
			}
			sx->identab[id + 3] = (int)mem_get_size(sx);

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TLabel\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TSwitch:
		{
			size_t oldbreak = context->adbreak;
			size_t oldcase = context->adcase;

			context->adbreak = 0;
			context->adcase = 0;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TSwitch\n", node_get_type(tree_get_node(sx)), local_tc);

			Expr_gen(sx, 0);
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
			printf("%i tc=%i: Stmt_gen TCase\n", node_get_type(tree_get_node(sx)), local_tc);

			Expr_gen(sx, 0);
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
			printf("%i tc=%i: Stmt_gen TDefault\n", node_get_type(tree_get_node(sx)), local_tc);

			Stmt_gen(sx, context);
			break;
		}
		case TBreak:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adbreak);
			context->adbreak = mem_get_size(sx) - 1;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TBreak\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TContinue:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adcont);
			context->adcont = mem_get_size(sx) - 1;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TContinue\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TReturnvoid:
		{
			tocode(sx, RETURNVOID);

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TReturnvoid\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TReturnval:
		{
			int d = node_get_arg(tree_get_node(sx), 0);

			local_tc++;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TReturnval\n", node_get_type(tree_get_node(sx)), local_tc);

			Expr_gen(sx, 0);
			tocode(sx, RETURNVAL);
			tocode(sx, d);
			break;
		}
		case TPrintid:
		{
			tocode(sx, PRINTID);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // ссылка в identtab

			local_tc++;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TPrintid\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TPrintf:
		{
			tocode(sx, PRINTF);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // общий размер того,
														   // что надо вывести
			local_tc++;
			
			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TPrintf\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case TGetid:
		{
			tocode(sx, GETID);
			tocode(sx, node_get_arg(tree_get_node(sx), 0)); // ссылка в identtab

			local_tc++;

			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen TGetid\n", node_get_type(tree_get_node(sx)), local_tc);
			break;
		}
		case SETMOTOR:
		{
			tree_next_node(sx);
			printf("%i tc=%i: Stmt_gen SETMOTOR\n", node_get_type(tree_get_node(sx)), local_tc);

			Expr_gen(sx, 0);
			Expr_gen(sx, 0);
			tocode(sx, SETMOTORC);
			break;
		}
		default:
		{
			local_tc--;
			printf("default %i tc=%i: Stmt_gen\n", node_get_type(tree_get_node(sx)), local_tc);
			Expr_gen(sx, 0);
			break;
		}
	}
}

void Struct_init_gen(syntax *const sx)
{
	int i;
	int n;


	if (node_get_type(tree_get_node(sx)) == TStructinit)
	{
		local_tc += 2;
		n = node_get_arg(tree_get_node(sx), 0);

		tree_next_node(sx);
		printf("%i tc=%i :Struct_init_gen\n", node_get_type(tree_get_node(sx)), local_tc);

		for (i = 0; i < n; i++)
		{
			Struct_init_gen(sx);
		}
		local_tc++; // TExprend

		tree_next_node(sx); // TExprend
		printf("%i tc=%i :Struct_init_gen TExprend\n", node_get_type(tree_get_node(sx)), local_tc);
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
	int element_len;
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

	local_tc += 7;
	element_len = sz_of(sx, telem);

	tree_next_node(sx);
	printf("%i tc=%i :Declid_gen\n", node_get_type(tree_get_node(sx)), local_tc);

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
				Struct_init_gen(sx);
				tocode(sx, COPY0STASS);
				tocode(sx, olddispl);
				tocode(sx, all); // общее кол-во слов
			}
			else
			{
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
				int i;
				int N;

				local_tc += 2;
				N = node_get_arg(tree_get_node(sx), 0);

				tree_next_node(sx);
				printf("%i tc=%i: compstmt_gen TDeclarr\n", node_get_type(tree_get_node(sx)), local_tc);

				for (i = 0; i < N; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TDeclid:
			{
				local_tc++;

				printf("TDeclid %i tc=%i: compstmt_gen\n", node_get_type(tree_get_node(sx)), local_tc);

				Declid_gen(sx);
				break;
			}
			default:
			{
				printf("default %i tc=%i: compstmt_gen\n", node_get_type(tree_get_node(sx)), local_tc);
				Stmt_gen(sx, context);
				break;
			}
		}
	}
	local_tc++; // TEnd

	tree_next_node(sx); // TEnd
	printf("%i tc=%i: compstmt_gen exit\n", node_get_type(tree_get_node(sx)), local_tc);
}

/** Генерация кодов */
int codegen(universal_io *const io, syntax *const sx)
{
	node root = node_get_root(sx);
	tree_set_node(sx, &root);

	ad context;

	int treesize = sx->tc;
	local_tc = 0;

	tree_next_node(sx);
	printf("%i tc=%i :codegen\n", node_get_type(tree_get_node(sx)), local_tc);
	while (local_tc < treesize)
	{
		local_tc++;
		switch (node_get_type(tree_get_node(sx)))
		{
			case TEnd:
				// printf("%i tc=%i :codegen TEnd\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			case TFuncdef:
			{
				int identref = node_get_arg(tree_get_node(sx), 0);
				int maxdispl = node_get_arg(tree_get_node(sx), 1);
				int fn = sx->identab[identref + 3];

				func_set(sx, fn, mem_get_size(sx));
				tocode(sx, FUNCBEG);
				tocode(sx, maxdispl);
				size_t old_pc = mem_get_size(sx);
				mem_increase(sx, 1);

				local_tc += 2;
				tree_next_node(sx);
				printf("%i tc=%i :codegen TBegin\n", node_get_type(tree_get_node(sx)), local_tc);

				local_tc++; 	

				tree_next_node(sx); // TBegin
				printf("%i tc=%i :codegen\n", node_get_type(tree_get_node(sx)), local_tc);

				compstmt_gen(sx, &context);
				mem_set(sx, old_pc, (int)mem_get_size(sx));
				break;
			}
			case TDeclarr:
			{
				int i;
				int N = node_get_arg(tree_get_node(sx), 0);

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i :codegen TDeclarr\n", node_get_type(tree_get_node(sx)), local_tc);

				for (i = 0; i < N; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				break;
			}
			case NOP:
			{
				tree_next_node(sx);
				printf("%i tc=%i :codegen NOP\n", node_get_type(tree_get_node(sx)), local_tc);

				break;
			}
			case TStructbeg:
			{
				tocode(sx, B);
				tocode(sx, 0);
				proc_set(sx, node_get_arg(tree_get_node(sx), 0), (int)mem_get_size(sx));

				local_tc++;

				tree_next_node(sx);
				printf("%i tc=%i :codegen TStructbeg\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			case TStructend:
			{
				int numproc = sx->tree[node_get_arg(tree_get_node(sx), 0) + 1];

				local_tc++;

				tocode(sx, STOP);
				mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));

				tree_next_node(sx);
				printf("%i tc=%i :codegen TStructend\n", node_get_type(tree_get_node(sx)), local_tc);
				break;
			}
			default:
			{
				printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", local_tc, sx->tree[local_tc - 2],
					   sx->tree[local_tc - 1]);
				break;
			}
		}
	}

	if (sx->wasmain == 0)
	{
		error(io, no_main_in_program);
		return -1;
	}
	tocode(sx, CALL1);
	tocode(sx, CALL2);
	tocode(sx, sx->identab[sx->wasmain + 3]);
	tocode(sx, STOP);

	return 0;
}

/** Вывод таблиц в файл */
void output_export(universal_io *const io, const syntax *const sx)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %i %i %zi %i %i %i\n", mem_get_size(sx), sx->funcnum, sx->id,
				   sx->rp, sx->md, sx->maxdisplg, sx->wasmain);

	for (size_t i = 0; i < mem_get_size(sx); i++)
	{
		uni_printf(io, "%i ", mem_get(sx, i));
	}
	uni_printf(io, "\n");

	for (int i = 0; i < sx->funcnum; i++)
	{
		uni_printf(io, "%i ", func_get(sx, i));
	}
	uni_printf(io, "\n");

	for (int i = 0; i < sx->id; i++)
	{
		uni_printf(io, "%i ", sx->identab[i]);
	}
	uni_printf(io, "\n");

	for (size_t i = 0; i < sx->rp; i++)
	{
		uni_printf(io, "%i ", sx->reprtab[i]);
	}

	for (int i = 0; i < sx->md; i++)
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

	int ret = codegen(io, sx);
	if (!ret)
	{
		output_export(io, sx);
	}

	return ret;
}
