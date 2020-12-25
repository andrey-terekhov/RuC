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
#include "defs.h"
#include "errors.h"
#include "uniprinter.h"
#include <stdlib.h>


typedef struct ad
{
	int adcont;
	int adbreak;
	int adcase;
} ad;


void Declid_gen(syntax *const sx);
void compstmt_gen(syntax *const sx, ad *const context);


void tocode(syntax *const sx, int c)
{
	// printf("tocode sx->tc=%i sx->pc %i) %i\n", sx->tc,
	// sx->pc, c);
	mem_add(sx, c);
}

void adbreakend(syntax *const sx, ad *const context)
{
	while (context->adbreak)
	{
		int r = mem_get(sx, context->adbreak);
		mem_set(sx, context->adbreak, sx->pc);
		context->adbreak = r;
	}
}

void adcontbeg(syntax *const sx, ad *const context, int ad)
{
	while (context->adcont != ad)
	{
		int r = mem_get(sx, context->adcont);
		mem_set(sx, context->adcont, ad);
		context->adcont = r;
	}
}

void adcontend(syntax *const sx, ad *const context)
{
	while (context->adcont != 0)
	{
		int r = mem_get(sx, context->adcont);
		mem_set(sx, context->adcont, sx->pc);
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
				sx->tree[sx->tree[sx->tc++]] = sx->pc++;
			}
			else if (c == ADLOGAND)
			{
				tocode(sx, _DOUBLE);
				tocode(sx, BE0);
				sx->tree[sx->tree[sx->tc++]] = sx->pc++;
			}
			else
			{
				tocode(sx, c);
				if (c == LOGOR || c == LOGAND)
				{
					mem_set(sx, sx->tree[sx->tc++], sx->pc);
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
		switch (op = sx->tree[sx->tc++])
		{
			case TIdent:
			{
				sx->anstdispl = sx->tree[sx->tc++];
				break;
			}
			case TIdenttoaddr:
			{
				tocode(sx, LA);
				tocode(sx, sx->anstdispl = sx->tree[sx->tc++]);
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
				int res;
				int i;

				tocode(sx, LI);
				tocode(sx, res = sx->pc + 4);
				tocode(sx, B);
				sx->pc += 2;
				for (i = 0; i < n; i++)
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
				mem_set(sx, res - 2, sx->pc);
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
				int adelse;
				int ad = 0;
				do
				{
					sx->tc++;
					tocode(sx, BE0);
					adelse = sx->pc++;
					Expr_gen(sx, 0); // then
					tocode(sx, B);
					mem_add(sx, ad);
					ad = sx->pc - 1;
					mem_set(sx, adelse, sx->pc);
					Expr_gen(sx, 1); // else или cond
				} while (sx->tree[sx->tc] == TCondexpr);

				while (ad)
				{
					int r = mem_get(sx, ad);
					mem_set(sx, ad, sx->pc);
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
		case EXITC:
		{
			tocode(sx, EXITC);
			break;
		}
		case TStructbeg:
		{
			tocode(sx, B);
			tocode(sx, 0);
			sx->iniprocs[sx->tree[sx->tc++]] = sx->pc;
			break;
		}
		case TStructend:
		{
			int numproc = sx->tree[sx->tree[sx->tc++] + 1];

			tocode(sx, STOP);
			mem_set(sx, sx->iniprocs[numproc] - 1, sx->pc);
			break;
		}
		case TBegin:
			compstmt_gen(sx, context);
			break;

		case TIf:
		{
			int elseref = sx->tree[sx->tc++];
			int ad;

			Expr_gen(sx, 0);
			tocode(sx, BE0);
			ad = sx->pc++;
			Stmt_gen(sx, context);
			if (elseref)
			{
				mem_set(sx, ad, sx->pc + 2);
				tocode(sx, B);
				ad = sx->pc++;
				Stmt_gen(sx, context);
			}
			mem_set(sx, ad, sx->pc);
			break;
		}
		case TWhile:
		{
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int ad = sx->pc;

			context->adcont = ad;
			Expr_gen(sx, 0);
			tocode(sx, BE0);
			mem_add(sx, 0);
			context->adbreak = sx->pc - 1;
			Stmt_gen(sx, context);
			adcontbeg(sx, context, ad);
			tocode(sx, B);
			tocode(sx, ad);
			adbreakend(sx, context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TDo:
		{
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int ad = sx->pc;

			context->adcont = context->adbreak = 0;
			Stmt_gen(sx, context);
			adcontend(sx, context);
			Expr_gen(sx, 0);
			tocode(sx, BNE0);
			tocode(sx, ad);
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
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int incrtc;
			int endtc;
			int initad;

			if (fromref)
			{
				Expr_gen(sx, 0); // init
			}

			initad = sx->pc;
			context->adcont = context->adbreak = 0;

			if (condref)
			{
				Expr_gen(sx, 0); // cond
				tocode(sx, BE0);
				mem_add(sx, sx->pc, 0);
				context->adbreak = sx->pc - 1;
			}
			incrtc = sx->tc;
			sx->tc = stmtref;
			Stmt_gen(sx, context); // ???? был 0
			adcontend(sx, context);

			if (incrref)
			{
				endtc = sx->tc;
				sx->tc = incrtc;
				Expr_gen(sx, 0); // incr
				sx->tc = endtc;
			}

			tocode(sx, B);
			tocode(sx, initad);
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
				sx->identab[id + 3] = -sx->pc;
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
					mem_set(sx, -a, sx->pc);
					a = r;
				}
			}
			sx->identab[id + 3] = sx->pc;
			break;
		}
		case TSwitch:
		{
			int oldbreak = context->adbreak;
			int oldcase = context->adcase;

			context->adbreak = 0;
			context->adcase = 0;
			Expr_gen(sx, 0);
			Stmt_gen(sx, context);
			if (context->adcase > 0)
			{
				sx->mem[context->adcase] = sx->pc;
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
				sx->mem[context->adcase] = sx->pc;
			}
			tocode(sx, _DOUBLE);
			Expr_gen(sx, 0);
			tocode(sx, EQEQ);
			tocode(sx, BE0);
			context->adcase = sx->pc++;
			Stmt_gen(sx, context);
			break;
		}
		case TDefault:
		{
			if (context->adcase)
			{
				sx->mem[context->adcase] = sx->pc;
			}
			context->adcase = 0;
			Stmt_gen(sx, context);
			break;
		}
		case TBreak:
		{
			tocode(sx, B);
			sx->mem[sx->pc] = context->adbreak;
			context->adbreak = sx->pc++;
			break;
		}
		case TContinue:
		{
			tocode(sx, B);
			sx->mem[sx->pc] = context->adcont;
			context->adcont = sx->pc++;
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
	element_len = sz_of(sx, telem);

	if (N == 0) // обычная переменная int a; или struct point p;
	{
		if (iniproc)
		{
			tocode(sx, STRUCTWITHARR);
			tocode(sx, olddispl);
			tocode(sx, sx->iniprocs[iniproc]);
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
		tocode(sx, sx->iniprocs[iniproc]);
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
int codegen(universal_io *const io, syntax *const sx)
{
	ad context;

	int treesize = sx->tc;
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
				int pred;

				func_set(sx, fn, sx->pc);
				tocode(sx, FUNCBEG);
				tocode(sx, maxdispl);
				pred = sx->pc++;
				sx->tc++; // TBegin
				compstmt_gen(sx, &context);
				sx->mem[pred] = sx->pc;
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
				sx->iniprocs[sx->tree[sx->tc++]] = sx->pc;
				break;
			}
			case TStructend:
			{
				int numproc = sx->tree[sx->tree[sx->tc++] + 1];

				tocode(sx, STOP);
				sx->mem[sx->iniprocs[numproc] - 1] = sx->pc;
				break;
			}
			default:
			{
				printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", sx->tc, sx->tree[sx->tc - 2],
					   sx->tree[sx->tc - 1]);
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

	uni_printf(io, "%i %i %i %i %i %i %i\n", sx->pc, sx->funcnum, sx->id,
				   sx->rp, sx->md, sx->maxdisplg, sx->wasmain);

	for (int i = 0; i < sx->pc; i++)
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

	for (int i = 0; i < sx->rp; i++)
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
