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
#include "global.h"
#include <stdlib.h>


void Declid_gen(compiler_context *context);
void compstmt_gen(compiler_context *context);


void tocode(compiler_context *context, int c)
{
	// printf("tocode context->tc=%i context->pc %i) %i\n", context->tc,
	// context->pc, c);
	context->mem[context->pc++] = c;
}

void adbreakend(compiler_context *context)
{
	while (context->adbreak)
	{
		int r = context->mem[context->adbreak];
		context->mem[context->adbreak] = context->pc;
		context->adbreak = r;
	}
}

void adcontbeg(compiler_context *context, int ad)
{
	while (context->adcont != ad)
	{
		int r = context->mem[context->adcont];
		context->mem[context->adcont] = ad;
		context->adcont = r;
	}
}

void adcontend(compiler_context *context)
{
	while (context->adcont != 0)
	{
		int r = context->mem[context->adcont];
		context->mem[context->adcont] = context->pc;
		context->adcont = r;
	}
}

void finalop(compiler_context *context)
{
	int c;

	while ((c = context->tree[context->tc]) > 9000)
	{
		context->tc++;
		if (c != NOP)
		{
			if (c == ADLOGOR)
			{
				tocode(context, _DOUBLE);
				tocode(context, BNE0);
				context->tree[context->tree[context->tc++]] = context->pc++;
			}
			else if (c == ADLOGAND)
			{
				tocode(context, _DOUBLE);
				tocode(context, BE0);
				context->tree[context->tree[context->tc++]] = context->pc++;
			}
			else
			{
				tocode(context, c);
				if (c == LOGOR || c == LOGAND)
				{
					context->mem[context->tree[context->tc++]] = context->pc;
				}
				else if (c == COPY00 || c == COPYST)
				{
					tocode(context, context->tree[context->tc++]); // d1
					tocode(context, context->tree[context->tc++]); // d2
					tocode(context, context->tree[context->tc++]); // длина
				}
				else if (c == COPY01 || c == COPY10 || c == COPY0ST || c == COPY0STASS)
				{
					tocode(context, context->tree[context->tc++]); // d1
					tocode(context, context->tree[context->tc++]); // длина
				}
				else if (c == COPY11 || c == COPY1ST || c == COPY1STASS)
				{
					tocode(context, context->tree[context->tc++]); // длина
				}
				else if ((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
						 (c >= ASSR && c <= DIVASSR) || (c >= ASSRV && c <= DIVASSRV) || (c >= POSTINC && c <= DEC) ||
						 (c >= POSTINCV && c <= DECV) || (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
				{
					tocode(context, context->tree[context->tc++]);
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
		switch (op = context->tree[context->tc++])
		{
			case TIdent:
			{
				context->anstdispl = context->tree[context->tc++];
				break;
			}
			case TIdenttoaddr:
			{
				tocode(context, LA);
				tocode(context, context->anstdispl = context->tree[context->tc++]);
				break;
			}
			case TIdenttoval:
			{
				tocode(context, LOAD);
				tocode(context, context->tree[context->tc++]);
				break;
			}
			case TIdenttovald:
			{
				tocode(context, LOADD);
				tocode(context, context->tree[context->tc++]);
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
				tocode(context, context->tree[context->tc++]);
				break;
			}
			case TConstd:
			{
				tocode(context, LID);
				tocode(context, context->tree[context->tc++]);
				tocode(context, context->tree[context->tc++]);
				break;
			}
			case TString:
			case TStringd:
			{
				int n = context->tree[context->tc++];
				int res;
				int i;

				tocode(context, LI);
				tocode(context, res = context->pc + 4);
				tocode(context, B);
				context->pc += 2;
				for (i = 0; i < n; i++)
				{
					if (op == TString)
					{
						tocode(context, context->tree[context->tc++]);
					}
					else
					{
						tocode(context, context->tree[context->tc++]);
						tocode(context, context->tree[context->tc++]);
					}
				}
				context->mem[res - 1] = n;
				context->mem[res - 2] = context->pc;
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
				int n = context->tree[context->tc++];
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
				int n = context->tree[context->tc++];
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
					   context->tree[context->tc++]); // продолжение в след case
			}
			case TSlice: // параметр - тип элемента
			{
				eltype = context->tree[context->tc++];
				Expr_gen(context, 0);
				tocode(context, SLICE);
				tocode(context, szof(context, eltype));
				if (eltype > 0 && context->modetab[eltype] == MARRAY)
				{
					tocode(context, LAT);
				}
				break;
			}
			case TSelect:
			{
				tocode(context, SELECT); // SELECT field_displ
				tocode(context, context->tree[context->tc++]);
				break;
			}
			case TPrint:
			{
				tocode(context, PRINT);
				tocode(context, context->tree[context->tc++]); // type
				break;
			}
			case TCall1:
			{
				int i;
				int n = context->tree[context->tc++];

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
				tocode(context, context->identab[context->tree[context->tc++] + 3]);
				break;
			}
			default:
			{
				context->tc--;
				break;
			}
		}

		finalop(context);

		if (context->tree[context->tc] == TCondexpr)
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
					context->tc++;
					tocode(context, BE0);
					adelse = context->pc++;
					Expr_gen(context, 0); // then
					tocode(context, B);
					context->mem[context->pc] = ad;
					ad = context->pc;
					context->mem[adelse] = ++context->pc;
					Expr_gen(context, 1); // else или cond
				} while (context->tree[context->tc] == TCondexpr);

				while (ad)
				{
					int r = context->mem[ad];
					context->mem[ad] = context->pc;
					ad = r;
				}
			}

			finalop(context);
		}
		if (context->tree[context->tc] == TExprend)
		{
			context->tc++;
			flagprim = 0;
		}
	}
	return wasstring;
}

void Stmt_gen(compiler_context *context)
{
	switch (context->tree[context->tc++])
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
			context->iniprocs[context->tree[context->tc++]] = context->pc;
			break;
		}
		case TStructend:
		{
			int numproc = context->tree[context->tree[context->tc++] + 1];

			tocode(context, STOP);
			context->mem[context->iniprocs[numproc] - 1] = context->pc;
			break;
		}
		case TBegin:
			compstmt_gen(context);
			break;

		case TIf:
		{
			int elseref = context->tree[context->tc++];
			int ad;

			Expr_gen(context, 0);
			tocode(context, BE0);
			ad = context->pc++;
			Stmt_gen(context);
			if (elseref)
			{
				context->mem[ad] = context->pc + 2;
				tocode(context, B);
				ad = context->pc++;
				Stmt_gen(context);
			}
			context->mem[ad] = context->pc;
			break;
		}
		case TWhile:
		{
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int ad = context->pc;

			context->adcont = ad;
			Expr_gen(context, 0);
			tocode(context, BE0);
			context->mem[context->pc] = 0;
			context->adbreak = context->pc++;
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
			int ad = context->pc;

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
			int fromref = context->tree[context->tc++];
			int condref = context->tree[context->tc++];
			int incrref = context->tree[context->tc++];
			int stmtref = context->tree[context->tc++];
			int oldbreak = context->adbreak;
			int oldcont = context->adcont;
			int incrtc;
			int endtc;
			int initad;

			if (fromref)
			{
				Expr_gen(context, 0); // init
			}

			initad = context->pc;
			context->adcont = context->adbreak = 0;

			if (condref)
			{
				Expr_gen(context, 0); // cond
				tocode(context, BE0);
				context->mem[context->pc] = 0;
				context->adbreak = context->pc++;
			}
			incrtc = context->tc;
			context->tc = stmtref;
			Stmt_gen(context); // ???? был 0
			adcontend(context);

			if (incrref)
			{
				endtc = context->tc;
				context->tc = incrtc;
				Expr_gen(context, 0); // incr
				context->tc = endtc;
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
			int id1 = context->tree[context->tc++];
			int a;
			int id = id1 > 0 ? id1 : -id1;

			tocode(context, B);
			if ((a = context->identab[id + 3]) > 0) // метка уже описана
			{
				tocode(context, a);
			}
			else // метка еще не описана
			{
				context->identab[id + 3] = -context->pc;
				tocode(context,
					   id1 < 0 ? 0 : a); // первый раз встретился переход на еще
										 // не описанную метку или нет
			}
			break;
		}
		case TLabel:
		{
			int id = context->tree[context->tc++];
			int a;

			if ((a = context->identab[id + 3]) < 0) // были переходы на метку
			{
				while (a) // проставить ссылку на метку во всех ранних переходах
				{
					int r = context->mem[-a];
					context->mem[-a] = context->pc;
					a = r;
				}
			}
			context->identab[id + 3] = context->pc;
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
				context->mem[context->adcase] = context->pc;
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
				context->mem[context->adcase] = context->pc;
			}
			tocode(context, _DOUBLE);
			Expr_gen(context, 0);
			tocode(context, EQEQ);
			tocode(context, BE0);
			context->adcase = context->pc++;
			Stmt_gen(context);
			break;
		}
		case TDefault:
		{
			if (context->adcase)
			{
				context->mem[context->adcase] = context->pc;
			}
			context->adcase = 0;
			Stmt_gen(context);
			break;
		}
		case TBreak:
		{
			tocode(context, B);
			context->mem[context->pc] = context->adbreak;
			context->adbreak = context->pc++;
			break;
		}
		case TContinue:
		{
			tocode(context, B);
			context->mem[context->pc] = context->adcont;
			context->adcont = context->pc++;
			break;
		}
		case TReturnvoid:
		{
			tocode(context, RETURNVOID);
			break;
		}
		case TReturnval:
		{
			int d = context->tree[context->tc++];

			Expr_gen(context, 0);
			tocode(context, RETURNVAL);
			tocode(context, d);
			break;
		}
		case TPrintid:
		{
			tocode(context, PRINTID);
			tocode(context, context->tree[context->tc++]); // ссылка в identtab
			break;
		}
		case TPrintf:
		{
			tocode(context, PRINTF);
			tocode(context, context->tree[context->tc++]); // общий размер того,
														   // что надо вывести
			break;
		}
		case TGetid:
		{
			tocode(context, GETID);
			tocode(context, context->tree[context->tc++]); // ссылка в identtab
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
			context->tc--;
			Expr_gen(context, 0);
			break;
		}
	}
}

void Struct_init_gen(compiler_context *context)
{
	int i;
	int n;

	if (context->tree[context->tc] == TStructinit)
	{
		context->tc++;
		n = context->tree[context->tc++];
		for (i = 0; i < n; i++)
		{
			Struct_init_gen(context);
		}
		context->tc++; // TExprend
	}
	else
	{
		Expr_gen(context, 0);
	}
}

void Declid_gen(compiler_context *context)
{
	int olddispl = context->tree[context->tc++];
	int telem = context->tree[context->tc++];
	int N = context->tree[context->tc++];
	int element_len;
	int all = context->tree[context->tc++];
	int iniproc = context->tree[context->tc++];
	int usual = context->tree[context->tc++];
	int instruct = context->tree[context->tc++];
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
			tocode(context, context->iniprocs[iniproc]);
		}
		if (all) // int a = или struct{} a =
		{
			if (telem > 0 && context->modetab[telem] == MSTRUCT)
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
		tocode(context, context->iniprocs[iniproc]);
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
	while (context->tree[context->tc] != TEnd)
	{
		switch (context->tree[context->tc])
		{
			case TDeclarr:
			{
				int i;
				int N;

				context->tc++;
				N = context->tree[context->tc++];
				for (i = 0; i < N; i++)
				{
					Expr_gen(context, 0);
				}
				break;
			}
			case TDeclid:
			{
				context->tc++;
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
	context->tc++;
}

void codegen(compiler_context *context)
{
	int treesize = context->tc;

	context->tc = 0;

	while (context->tc < treesize)
	{
		switch (context->tree[context->tc++])
		{
			case TEnd:
				break;
			case TFuncdef:
			{
				int identref = context->tree[context->tc++];
				int maxdispl = context->tree[context->tc++];
				int fn = context->identab[identref + 3];
				int pred;

				context->functions[fn] = context->pc;
				tocode(context, FUNCBEG);
				tocode(context, maxdispl);
				pred = context->pc++;
				context->tc++; // TBegin
				compstmt_gen(context);
				context->mem[pred] = context->pc;
				break;
			}
			case TDeclarr:
			{
				int i;
				int N = context->tree[context->tc++];

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
				context->iniprocs[context->tree[context->tc++]] = context->pc;
				break;
			}
			case TStructend:
			{
				int numproc = context->tree[context->tree[context->tc++] + 1];

				tocode(context, STOP);
				context->mem[context->iniprocs[numproc] - 1] = context->pc;
				break;
			}
			default:
			{
				printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", context->tc, context->tree[context->tc - 2],
					   context->tree[context->tc - 1]);
				break;
			}
		}
	}

	if (context->wasmain == 0)
	{
		error(&context->io, no_main_in_program);
		return;
	}
	tocode(context, CALL1);
	tocode(context, CALL2);
	tocode(context, context->identab[context->wasmain + 3]);
	tocode(context, STOP);
}
