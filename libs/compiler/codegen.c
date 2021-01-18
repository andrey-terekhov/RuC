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
	// printf("tocode sx->tc=%zi sx->pc %zi) %i\n", sx->tc,
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

	while ((c = sx->tree[sx->tc]) > 9000)
	{
		sx->tc++;
		if (c != NOP)
		{
			if (c == ADLOGOR)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BNE0);
				sx->tree[sx->tree[sx->tc++]] = (int)mem_get_size(sx);
				mem_increase(sx, 1);
			}
			else if (c == ADLOGAND)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BE0);
				sx->tree[sx->tree[sx->tc++]] = (int)mem_get_size(sx);
				mem_increase(sx, 1);
			}
			else
			{
				tocode(sx, c);
				if (c == LOGOR || c == LOGAND)
				{
					mem_set(sx, sx->tree[sx->tc++], (int)mem_get_size(sx));
				}
				else if (c == COPY00 || c == COPYST)
				{
					tocode(sx, sx->tree[sx->tc++]); // d1
					tocode(sx, sx->tree[sx->tc++]); // d2
					tocode(sx, sx->tree[sx->tc++]); // длина
				}
				else if (c == COPY01 || c == COPY10 || c == COPY0ST || c == COPY0STASS)
				{
					tocode(sx, sx->tree[sx->tc++]); // d1
					tocode(sx, sx->tree[sx->tc++]); // длина
				}
				else if (c == COPY11 || c == COPY1ST || c == COPY1STASS)
				{
					tocode(sx, sx->tree[sx->tc++]); // длина
				}
				else if ((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
						 (c >= ASSR && c <= DIVASSR) || (c >= ASSRV && c <= DIVASSRV) || (c >= POSTINC && c <= DEC) ||
						 (c >= POSTINCV && c <= DECV) || (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
				{
					tocode(sx, sx->tree[sx->tc++]);
				}
			}
		}
	}
}

int Expr_gen(syntax *const sx, int incond)
{
	int flagprim = 1;
	int eltype;
	int wasstring = 0;
	int op;

	while (flagprim)
	{
		switch (op = sx->tree[sx->tc++])
		{
			case TIdent:
			{
				sx->tc++;
				break;
			}
			case TIdenttoaddr:
			{
				tocode(sx, LA);
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TIdenttoval:
			{
				tocode(sx, LOAD);
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TIdenttovald:
			{
				tocode(sx, LOADD);
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TAddrtoval:
			{
				tocode(sx, LAT);
				break;
			}
			case TAddrtovald:
			{
				tocode(sx, LATD);
				break;
			}
			case TConst:
			{
				tocode(sx, LI);
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TConstd:
			{
				tocode(sx, LID);
				tocode(sx, sx->tree[sx->tc++]);
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TString:
			case TStringd:
			{
				int n = sx->tree[sx->tc++];

				tocode(sx, LI);
				size_t res = mem_get_size(sx) + 4;
				tocode(sx, (int)res);
				tocode(sx, B);
				mem_increase(sx, 2);
				for (int i = 0; i < n; i++)
				{
					if (op == TString)
					{
						tocode(sx, sx->tree[sx->tc++]);
					}
					else
					{
						tocode(sx, sx->tree[sx->tc++]);
						tocode(sx, sx->tree[sx->tc++]);
					}
				}
				mem_set(sx, res - 1, n);
				mem_set(sx, res - 2, (int)mem_get_size(sx));
				wasstring = 1;
				break;
			}
			case TDeclid:
			{
				Declid_gen(sx);
				break;
			}
			case TBeginit:
			{
				int n = sx->tree[sx->tc++];
				int i;

				tocode(sx, BEGINIT);
				tocode(sx, n);
				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TStructinit:
			{
				int n = sx->tree[sx->tc++];
				int i;

				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TSliceident:
			{
				tocode(sx,
					   LOAD); // параметры - смещение идента и тип элемента
				tocode(sx,
					   sx->tree[sx->tc++]); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				eltype = sx->tree[sx->tc++];
				Expr_gen(sx, 0);
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
				tocode(sx, sx->tree[sx->tc++]);
				break;
			}
			case TPrint:
			{
				tocode(sx, PRINT);
				tocode(sx, sx->tree[sx->tc++]); // type
				break;
			}
			case TCall1:
			{
				int i;
				int n = sx->tree[sx->tc++];

				tocode(sx, CALL1);
				for (i = 0; i < n; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TCall2:
			{
				tocode(sx, CALL2);
				tocode(sx, sx->identab[sx->tree[sx->tc++] + 3]);
				break;
			}
			default:
			{
				sx->tc--;
				break;
			}
		}

		finalop(sx);

		if (sx->tree[sx->tc] == TCondexpr)
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
					sx->tc++;
					tocode(sx, BE0);
					size_t adelse = mem_get_size(sx);
					mem_increase(sx, 1);
					Expr_gen(sx, 0); // then
					tocode(sx, B);
					mem_add(sx, (int)ad);
					ad = mem_get_size(sx) - 1;
					mem_set(sx, adelse, (int)mem_get_size(sx));
					Expr_gen(sx, 1); // else или cond
				} while (sx->tree[sx->tc] == TCondexpr);

				while (ad)
				{
					int r = mem_get(sx, ad);
					mem_set(sx, ad, (int)mem_get_size(sx));
					ad = r;
				}
			}

			finalop(sx);
		}
		if (sx->tree[sx->tc] == TExprend)
		{
			sx->tc++;
			flagprim = 0;
		}
	}
	return wasstring;
}

void Stmt_gen(syntax *const sx, ad *const context)
{
	switch (sx->tree[sx->tc++])
	{
		case NOP:
		{
			break;
		}
		case CREATEDIRECTC:
		{
			tocode(sx, CREATEDIRECTC);
			break;
		}
		case EXITDIRECTC:
		case EXITC:
		{
			tocode(sx, EXITC);
			break;
		}
		case TStructbeg:
		{
			tocode(sx, B);
			tocode(sx, 0);
			proc_set(sx, sx->tree[sx->tc++], (int)mem_get_size(sx));
			break;
		}
		case TStructend:
		{
			int numproc = sx->tree[sx->tree[sx->tc++] + 1];

			tocode(sx, STOP);
			mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));
			break;
		}
		case TBegin:
			compstmt_gen(sx, context);
			break;

		case TIf:
		{
			int elseref = sx->tree[sx->tc++];

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
			int fromref = sx->tree[sx->tc++];
			int condref = sx->tree[sx->tc++];
			int incrref = sx->tree[sx->tc++];
			int stmtref = sx->tree[sx->tc++];
			size_t oldbreak = context->adbreak;
			size_t oldcont = context->adcont;

			if (fromref)
			{
				Expr_gen(sx, 0); // init
			}

			size_t initad = mem_get_size(sx);
			context->adcont = context->adbreak = 0;

			if (condref)
			{
				Expr_gen(sx, 0); // cond
				tocode(sx, BE0);
				context->adbreak = mem_get_size(sx);
				mem_add(sx, 0);	
			}
			size_t incrtc = sx->tc;
			sx->tc = stmtref;
			Stmt_gen(sx, context); // ???? был 0
			adcontend(sx, context);

			if (incrref)
			{
				size_t endtc = sx->tc;
				sx->tc = incrtc;
				Expr_gen(sx, 0); // incr
				sx->tc = endtc;
			}

			tocode(sx, B);
			tocode(sx, (int)initad);
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TGoto:
		{
			int id1 = sx->tree[sx->tc++];
			int a;
			int id = id1 > 0 ? id1 : -id1;

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
			break;
		}
		case TLabel:
		{
			int id = sx->tree[sx->tc++];
			int a;

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
			break;
		}
		case TSwitch:
		{
			size_t oldbreak = context->adbreak;
			size_t oldcase = context->adcase;

			context->adbreak = 0;
			context->adcase = 0;
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
			Stmt_gen(sx, context);
			break;
		}
		case TBreak:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adbreak);
			context->adbreak = mem_get_size(sx) - 1;
			break;
		}
		case TContinue:
		{
			tocode(sx, B);
			mem_add(sx, (int)context->adcont);
			context->adcont = mem_get_size(sx) - 1;
			break;
		}
		case TReturnvoid:
		{
			tocode(sx, RETURNVOID);
			break;
		}
		case TReturnval:
		{
			int d = sx->tree[sx->tc++];

			Expr_gen(sx, 0);
			tocode(sx, RETURNVAL);
			tocode(sx, d);
			break;
		}
		case TPrintid:
		{
			tocode(sx, PRINTID);
			tocode(sx, sx->tree[sx->tc++]); // ссылка в identtab
			break;
		}
		case TPrintf:
		{
			tocode(sx, PRINTF);
			tocode(sx, sx->tree[sx->tc++]); // общий размер того,
														   // что надо вывести
			break;
		}
		case TGetid:
		{
			tocode(sx, GETID);
			tocode(sx, sx->tree[sx->tc++]); // ссылка в identtab
			break;
		}
		case SETMOTOR:
		{
			Expr_gen(sx, 0);
			Expr_gen(sx, 0);
			tocode(sx, SETMOTORC);
			break;
		}
		default:
		{
			sx->tc--;
			Expr_gen(sx, 0);
			break;
		}
	}
}

void Struct_init_gen(syntax *const sx)
{
	int i;
	int n;

	if (sx->tree[sx->tc] == TStructinit)
	{
		sx->tc++;
		n = sx->tree[sx->tc++];
		for (i = 0; i < n; i++)
		{
			Struct_init_gen(sx);
		}
		sx->tc++; // TExprend
	}
	else
	{
		Expr_gen(sx, 0);
	}
}

void Declid_gen(syntax *const sx)
{
	int olddispl = sx->tree[sx->tc++];
	int telem = sx->tree[sx->tc++];
	int N = sx->tree[sx->tc++];
	int element_len;
	int all = sx->tree[sx->tc++];
	int iniproc = sx->tree[sx->tc++];
	int usual = sx->tree[sx->tc++];
	int instruct = sx->tree[sx->tc++];
	// all - общее кол-во слов в структуре
	// для массивов есть еще usual // == 0 с пустыми границами,
	// == 1 без пустых границ,
	// all == 0 нет инициализатора,
	// all == 1 есть инициализатор
	// all == 2 есть инициализатор только из строк
	element_len = size_of(sx, telem);

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
	while (sx->tree[sx->tc] != TEnd)
	{
		switch (sx->tree[sx->tc])
		{
			case TDeclarr:
			{
				int i;
				int N;

				sx->tc++;
				N = sx->tree[sx->tc++];
				for (i = 0; i < N; i++)
				{
					Expr_gen(sx, 0);
				}
				break;
			}
			case TDeclid:
			{
				sx->tc++;
				Declid_gen(sx);
				break;
			}
			default:
			{
				Stmt_gen(sx, context);
				break;
			}
		}
	}
	sx->tc++;
}

/** Генерация кодов */
int codegen(syntax *const sx)
{
	ad context;

	size_t treesize = sx->tc;
	sx->tc = 0;

	while (sx->tc < treesize)
	{
		switch (sx->tree[sx->tc++])
		{
			case TEnd:
				break;
			case TFuncdef:
			{
				int identref = sx->tree[sx->tc++];
				int maxdispl = sx->tree[sx->tc++];
				int fn = sx->identab[identref + 3];

				func_set(sx, fn, mem_get_size(sx));
				tocode(sx, FUNCBEG);
				tocode(sx, maxdispl);
				size_t old_pc = mem_get_size(sx);
				mem_increase(sx, 1);
				sx->tc++; // TBegin
				compstmt_gen(sx, &context);
				mem_set(sx, old_pc, (int)mem_get_size(sx));
				break;
			}
			case TDeclarr:
			{
				int i;
				int N = sx->tree[sx->tc++];

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
				break;
			}
			case TStructbeg:
			{
				tocode(sx, B);
				tocode(sx, 0);
				proc_set(sx, sx->tree[sx->tc++], (int)mem_get_size(sx));
				break;
			}
			case TStructend:
			{
				int numproc = sx->tree[sx->tree[sx->tc++] + 1];

				tocode(sx, STOP);
				mem_set(sx, proc_get(sx, numproc) - 1, (int)mem_get_size(sx));
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
	tocode(sx, sx->identab[sx->main_ref + 3]);
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
