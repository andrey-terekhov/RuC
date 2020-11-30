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
#include "errors.h"
#include "extdecl.h"
#include "defs.h"
#include <stdlib.h>


void Declid_gen(compiler_context *context);
void compstmt_gen(compiler_context *context);


void tocode(compiler_context *context, int c)
{
	// printf("tocode context->sx->tc=%i context->sx->pc %i) %i\n", context->sx->tc,
	// context->sx->pc, c);
	context->sx->mem[context->sx->pc++] = c;
}

void adbreakend(compiler_context *context)
{
	while (context->adbreak)
	{
		int r = context->sx->mem[context->adbreak];
		context->sx->mem[context->adbreak] = context->sx->pc;
		context->adbreak = r;
	}
}

void adcontbeg(compiler_context *context, int ad)
{
	while (context->adcont != ad)
	{
		int r = context->sx->mem[context->adcont];
		context->sx->mem[context->adcont] = ad;
		context->adcont = r;
	}
}

void adcontend(compiler_context *context)
{
	while (context->adcont != 0)
	{
		int r = context->sx->mem[context->adcont];
		context->sx->mem[context->adcont] = context->sx->pc;
		context->adcont = r;
	}
}

void finalop(compiler_context *context)
{
	int c;

	while ((c = context->sx->tree[context->sx->tc]) > 9000)
	{
		context->sx->tc++;
		if (c != NOP)
		{
			if (c == ADLOGOR)
			{
				tocode(context, _DOUBLE);
				tocode(context, BNE0);
				context->sx->tree[context->sx->tree[context->sx->tc++]] = context->sx->pc++;
			}
			else if (c == ADLOGAND)
			{
				tocode(context, _DOUBLE);
				tocode(context, BE0);
				context->sx->tree[context->sx->tree[context->sx->tc++]] = context->sx->pc++;
			}
			else
			{
				tocode(context, c);
				if (c == LOGOR || c == LOGAND)
				{
					context->sx->mem[context->sx->tree[context->sx->tc++]] = context->sx->pc;
				}
				else if (c == COPY00 || c == COPYST)
				{
					tocode(context, context->sx->tree[context->sx->tc++]); // d1
					tocode(context, context->sx->tree[context->sx->tc++]); // d2
					tocode(context, context->sx->tree[context->sx->tc++]); // длина
				}
				else if (c == COPY01 || c == COPY10 || c == COPY0ST || c == COPY0STASS)
				{
					tocode(context, context->sx->tree[context->sx->tc++]); // d1
					tocode(context, context->sx->tree[context->sx->tc++]); // длина
				}
				else if (c == COPY11 || c == COPY1ST || c == COPY1STASS)
				{
					tocode(context, context->sx->tree[context->sx->tc++]); // длина
				}
				else if ((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
						 (c >= ASSR && c <= DIVASSR) || (c >= ASSRV && c <= DIVASSRV) || (c >= POSTINC && c <= DEC) ||
						 (c >= POSTINCV && c <= DECV) || (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
				{
					tocode(context, context->sx->tree[context->sx->tc++]);
				}
			}
		}
	}
}

int Expr_gen(compiler_context *context, int incond)
{
	int flagprim = 1;
	int eltype;
	int wasstring = 0;
	int op;

	while (flagprim)
	{
		switch (op = context->sx->tree[context->sx->tc++])
		{
			case TIdent:
			{
				context->sx->anstdispl = context->sx->tree[context->sx->tc++];
				break;
			}
			case TIdenttoaddr:
			{
				tocode(context, LA);
				tocode(context, context->sx->anstdispl = context->sx->tree[context->sx->tc++]);
				break;
			}
			case TIdenttoval:
			{
				tocode(context, LOAD);
				tocode(context, context->sx->tree[context->sx->tc++]);
				break;
			}
			case TIdenttovald:
			{
				tocode(context, LOADD);
				tocode(context, context->sx->tree[context->sx->tc++]);
				break;
			}
			case TAddrtoval:
			{
				tocode(context, LAT);
				break;
			}
			case TAddrtovald:
			{
				tocode(context, LATD);
				break;
			}
			case TConst:
			{
				tocode(context, LI);
				tocode(context, context->sx->tree[context->sx->tc++]);
				break;
			}
			case TConstd:
			{
				tocode(context, LID);
				tocode(context, context->sx->tree[context->sx->tc++]);
				tocode(context, context->sx->tree[context->sx->tc++]);
				break;
			}
			case TString:
			case TStringd:
			{
				int n = context->sx->tree[context->sx->tc++];
				int res;
				int i;

				tocode(context, LI);
				tocode(context, res = context->sx->pc + 4);
				tocode(context, B);
				context->sx->pc += 2;
				for (i = 0; i < n; i++)
				{
					if (op == TString)
					{
						tocode(context, context->sx->tree[context->sx->tc++]);
					}
					else
					{
						tocode(context, context->sx->tree[context->sx->tc++]);
						tocode(context, context->sx->tree[context->sx->tc++]);
					}
				}
				context->sx->mem[res - 1] = n;
				context->sx->mem[res - 2] = context->sx->pc;
				wasstring = 1;
				break;
			}
			case TDeclid:
			{
				Declid_gen(context);
				break;
			}
			case TBeginit:
			{
				int n = context->sx->tree[context->sx->tc++];
				int i;

				tocode(context, BEGINIT);
				tocode(context, n);
				for (i = 0; i < n; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TStructinit:
			{
				int n = context->sx->tree[context->sx->tc++];
				int i;

				for (i = 0; i < n; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TSliceident:
			{
				tocode(context,
					   LOAD); // параметры - смещение идента и тип элемента
				tocode(context,
					   context->sx->tree[context->sx->tc++]); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				eltype = context->sx->tree[context->sx->tc++];
				Expr_gen(context, 0);
				tocode(context, SLICE);
				tocode(context, szof(context, eltype));
				if (eltype > 0 && context->sx->modetab[eltype] == MARRAY)
				{
					tocode(context, LAT);
				}
				break;
			}
			case TSelect:
			{
				tocode(context, SELECT); // SELECT field_displ
				tocode(context, context->sx->tree[context->sx->tc++]);
				break;
			}
			case TPrint:
			{
				tocode(context, PRINT);
				tocode(context, context->sx->tree[context->sx->tc++]); // type
				break;
			}
			case TCall1:
			{
				int i;
				int n = context->sx->tree[context->sx->tc++];

				tocode(context, CALL1);
				for (i = 0; i < n; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TCall2:
			{
				tocode(context, CALL2);
				tocode(context, context->sx->identab[context->sx->tree[context->sx->tc++] + 3]);
				break;
			}
			default:
			{
				context->sx->tc--;
				break;
			}
		}

		finalop(context);

		if (context->sx->tree[context->sx->tc] == TCondexpr)
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
					context->sx->tc++;
					tocode(context, BE0);
					adelse = context->sx->pc++;
					Expr_gen(context, 0); // then
					tocode(context, B);
					context->sx->mem[context->sx->pc] = ad;
					ad = context->sx->pc;
					context->sx->mem[adelse] = ++context->sx->pc;
					Expr_gen(context, 1); // else или cond
				} while (context->sx->tree[context->sx->tc] == TCondexpr);

				while (ad)
				{
					int r = context->sx->mem[ad];
					context->sx->mem[ad] = context->sx->pc;
					ad = r;
				}
			}

			finalop(context);
		}
		if (context->sx->tree[context->sx->tc] == TExprend)
		{
			context->sx->tc++;
			flagprim = 0;
		}
	}
	return wasstring;
}

void Stmt_gen(compiler_context *context)
{
	switch (context->sx->tree[context->sx->tc++])
	{
		case NOP:
		{
			break;
		}
		case CREATEDIRECTC:
		{
			tocode(context, CREATEDIRECTC);
			break;
		}
		case EXITC:
		{
			tocode(context, EXITC);
			break;
		}
		case TStructbeg:
		{
			tocode(context, B);
			tocode(context, 0);
			context->sx->iniprocs[context->sx->tree[context->sx->tc++]] = context->sx->pc;
			break;
		}
		case TStructend:
		{
			int numproc = context->sx->tree[context->sx->tree[context->sx->tc++] + 1];

			tocode(context, STOP);
			context->sx->mem[context->sx->iniprocs[numproc] - 1] = context->sx->pc;
			break;
		}
		case TBegin:
			compstmt_gen(context);
			break;

		case TIf:
		{
			int elseref = context->sx->tree[context->sx->tc++];
			int ad;

			Expr_gen(context, 0);
			tocode(context, BE0);
			ad = context->sx->pc++;
			Stmt_gen(context);
			if (elseref)
			{
				context->sx->mem[ad] = context->sx->pc + 2;
				tocode(context, B);
				ad = context->sx->pc++;
				Stmt_gen(context);
			}
			context->sx->mem[ad] = context->sx->pc;
			break;
		}
		case TWhile:
		{
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int ad = context->sx->pc;

			context->adcont = ad;
			Expr_gen(context, 0);
			tocode(context, BE0);
			context->sx->mem[context->sx->pc] = 0;
			context->adbreak = context->sx->pc++;
			Stmt_gen(context);
			adcontbeg(context, ad);
			tocode(context, B);
			tocode(context, ad);
			adbreakend(context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TDo:
		{
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int ad = context->sx->pc;

			context->adcont = context->adbreak = 0;
			Stmt_gen(context);
			adcontend(context);
			Expr_gen(context, 0);
			tocode(context, BNE0);
			tocode(context, ad);
			adbreakend(context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TFor:
		{
			int fromref = context->sx->tree[context->sx->tc++];
			int condref = context->sx->tree[context->sx->tc++];
			int incrref = context->sx->tree[context->sx->tc++];
			int stmtref = context->sx->tree[context->sx->tc++];
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int incrtc;
			int endtc;
			int initad;

			if (fromref)
			{
				Expr_gen(context, 0); // init
			}

			initad = context->sx->pc;
			context->adcont = context->adbreak = 0;

			if (condref)
			{
				Expr_gen(context, 0); // cond
				tocode(context, BE0);
				context->sx->mem[context->sx->pc] = 0;
				context->adbreak = context->sx->pc++;
			}
			incrtc = context->sx->tc;
			context->sx->tc = stmtref;
			Stmt_gen(context); // ???? был 0
			adcontend(context);

			if (incrref)
			{
				endtc = context->sx->tc;
				context->sx->tc = incrtc;
				Expr_gen(context, 0); // incr
				context->sx->tc = endtc;
			}

			tocode(context, B);
			tocode(context, initad);
			adbreakend(context);
			context->adbreak = oldbreak;
			context->adcont = oldcont;
			break;
		}
		case TGoto:
		{
			int id1 = context->sx->tree[context->sx->tc++];
			int a;
			int id = id1 > 0 ? id1 : -id1;

			tocode(context, B);
			if ((a = context->sx->identab[id + 3]) > 0) // метка уже описана
			{
				tocode(context, a);
			}
			else // метка еще не описана
			{
				context->sx->identab[id + 3] = -context->sx->pc;
				tocode(context,
					   id1 < 0 ? 0 : a); // первый раз встретился переход на еще
										 // не описанную метку или нет
			}
			break;
		}
		case TLabel:
		{
			int id = context->sx->tree[context->sx->tc++];
			int a;

			if ((a = context->sx->identab[id + 3]) < 0) // были переходы на метку
			{
				while (a) // проставить ссылку на метку во всех ранних переходах
				{
					int r = context->sx->mem[-a];
					context->sx->mem[-a] = context->sx->pc;
					a = r;
				}
			}
			context->sx->identab[id + 3] = context->sx->pc;
			break;
		}
		case TSwitch:
		{
			int oldbreak = context->adbreak;
			int oldcase = context->adcase;

			context->adbreak = 0;
			context->adcase = 0;
			Expr_gen(context, 0);
			Stmt_gen(context);
			if (context->adcase > 0)
			{
				context->sx->mem[context->adcase] = context->sx->pc;
			}
			context->adcase = oldcase;
			adbreakend(context);
			context->adbreak = oldbreak;
			break;
		}
		case TCase:
		{
			if (context->adcase)
			{
				context->sx->mem[context->adcase] = context->sx->pc;
			}
			tocode(context, _DOUBLE);
			Expr_gen(context, 0);
			tocode(context, EQEQ);
			tocode(context, BE0);
			context->adcase = context->sx->pc++;
			Stmt_gen(context);
			break;
		}
		case TDefault:
		{
			if (context->adcase)
			{
				context->sx->mem[context->adcase] = context->sx->pc;
			}
			context->adcase = 0;
			Stmt_gen(context);
			break;
		}
		case TBreak:
		{
			tocode(context, B);
			context->sx->mem[context->sx->pc] = context->adbreak;
			context->adbreak = context->sx->pc++;
			break;
		}
		case TContinue:
		{
			tocode(context, B);
			context->sx->mem[context->sx->pc] = context->adcont;
			context->adcont = context->sx->pc++;
			break;
		}
		case TReturnvoid:
		{
			tocode(context, RETURNVOID);
			break;
		}
		case TReturnval:
		{
			int d = context->sx->tree[context->sx->tc++];

			Expr_gen(context, 0);
			tocode(context, RETURNVAL);
			tocode(context, d);
			break;
		}
		case TPrintid:
		{
			tocode(context, PRINTID);
			tocode(context, context->sx->tree[context->sx->tc++]); // ссылка в identtab
			break;
		}
		case TPrintf:
		{
			tocode(context, PRINTF);
			tocode(context, context->sx->tree[context->sx->tc++]); // общий размер того,
														   // что надо вывести
			break;
		}
		case TGetid:
		{
			tocode(context, GETID);
			tocode(context, context->sx->tree[context->sx->tc++]); // ссылка в identtab
			break;
		}
		case SETMOTOR:
		{
			Expr_gen(context, 0);
			Expr_gen(context, 0);
			tocode(context, SETMOTORC);
			break;
		}
		default:
		{
			context->sx->tc--;
			Expr_gen(context, 0);
			break;
		}
	}
}

void Struct_init_gen(compiler_context *context)
{
	int i;
	int n;

	if (context->sx->tree[context->sx->tc] == TStructinit)
	{
		context->sx->tc++;
		n = context->sx->tree[context->sx->tc++];
		for (i = 0; i < n; i++)
		{
			Struct_init_gen(context);
		}
		context->sx->tc++; // TExprend
	}
	else
	{
		Expr_gen(context, 0);
	}
}

void Declid_gen(compiler_context *context)
{
	int olddispl = context->sx->tree[context->sx->tc++];
	int telem = context->sx->tree[context->sx->tc++];
	int N = context->sx->tree[context->sx->tc++];
	int element_len;
	int all = context->sx->tree[context->sx->tc++];
	int iniproc = context->sx->tree[context->sx->tc++];
	int usual = context->sx->tree[context->sx->tc++];
	int instruct = context->sx->tree[context->sx->tc++];
	// all - общее кол-во слов в структуре
	// для массивов есть еще usual // == 0 с пустыми границами,
	// == 1 без пустых границ,
	// all == 0 нет инициализатора,
	// all == 1 есть инициализатор
	// all == 2 есть инициализатор только из строк
	element_len = szof(context, telem);

	if (N == 0) // обычная переменная int a; или struct point p;
	{
		if (iniproc)
		{
			tocode(context, STRUCTWITHARR);
			tocode(context, olddispl);
			tocode(context, context->sx->iniprocs[iniproc]);
		}
		if (all) // int a = или struct{} a =
		{
			if (telem > 0 && context->sx->modetab[telem] == MSTRUCT)
			{
				Struct_init_gen(context);
				tocode(context, COPY0STASS);
				tocode(context, olddispl);
				tocode(context, all); // общее кол-во слов
			}
			else
			{
				Expr_gen(context, 0);
				tocode(context, telem == LFLOAT ? ASSRV : ASSV);
				tocode(context, olddispl);
			}
		}
	}
	else // Обработка массива int a[N1]...[NN] =
	{
		tocode(context, DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN
								 // уже лежат на стеке
		tocode(context, all == 0 ? N : abs(N) - 1);
		tocode(context, element_len);
		tocode(context, olddispl);
		tocode(context, context->sx->iniprocs[iniproc]);
		tocode(context, usual);
		tocode(context, all);
		tocode(context, instruct);

		if (all) // all == 1, если есть инициализация массива
		{
			Expr_gen(context, 0);
			tocode(context, ARRINIT); // ARRINIT N d all displ usual
			tocode(context, abs(N));
			tocode(context, element_len);
			tocode(context, olddispl);
			tocode(context, usual); // == 0 с пустыми границами
									// == 1 без пустых границ и без иниц
		}
	}
}

void compstmt_gen(compiler_context *context)
{
	while (context->sx->tree[context->sx->tc] != TEnd)
	{
		switch (context->sx->tree[context->sx->tc])
		{
			case TDeclarr:
			{
				int i;
				int N;

				context->sx->tc++;
				N = context->sx->tree[context->sx->tc++];
				for (i = 0; i < N; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TDeclid:
			{
				context->sx->tc++;
				Declid_gen(context);
				break;
			}
			default:
			{
				Stmt_gen(context);
				break;
			}
		}
	}
	context->sx->tc++;
}

void codegen(compiler_context *context)
{
	int treesize = context->sx->tc;

	context->sx->tc = 0;

	while (context->sx->tc < treesize)
	{
		switch (context->sx->tree[context->sx->tc++])
		{
			case TEnd:
				break;
			case TFuncdef:
			{
				int identref = context->sx->tree[context->sx->tc++];
				int maxdispl = context->sx->tree[context->sx->tc++];
				int fn = context->sx->identab[identref + 3];
				int pred;

				context->sx->functions[fn] = context->sx->pc;
				tocode(context, FUNCBEG);
				tocode(context, maxdispl);
				pred = context->sx->pc++;
				context->sx->tc++; // TBegin
				compstmt_gen(context);
				context->sx->mem[pred] = context->sx->pc;
				break;
			}
			case TDeclarr:
			{
				int i;
				int N = context->sx->tree[context->sx->tc++];

				for (i = 0; i < N; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TDeclid:
			{
				Declid_gen(context);
				break;
			}
			case NOP:
			{
				break;
			}
			case TStructbeg:
			{
				tocode(context, B);
				tocode(context, 0);
				context->sx->iniprocs[context->sx->tree[context->sx->tc++]] = context->sx->pc;
				break;
			}
			case TStructend:
			{
				int numproc = context->sx->tree[context->sx->tree[context->sx->tc++] + 1];

				tocode(context, STOP);
				context->sx->mem[context->sx->iniprocs[numproc] - 1] = context->sx->pc;
				break;
			}
			default:
			{
				printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", context->sx->tc, context->sx->tree[context->sx->tc - 2],
					   context->sx->tree[context->sx->tc - 1]);
				break;
			}
		}
	}

	if (context->sx->wasmain == 0)
	{
		error(context->io, no_main_in_program);
		return;
	}
	tocode(context, CALL1);
	tocode(context, CALL2);
	tocode(context, context->sx->identab[context->sx->wasmain + 3]);
	tocode(context, STOP);
}
