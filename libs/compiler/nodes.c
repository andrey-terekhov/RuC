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

#include "nodes.h"
#include "stdio.h"

instruction_t node_to_instruction(const node_t node)
{
	switch (node)
	{
		case ND_NULL:			return IC_NOP;
		case ND_WIDEN:			return IC_WIDEN;
		case ND_WIDEN1:			return IC_WIDEN1;
		case ND_ROWING:			return IC_ROWING;
		case ND_ROWINGD:		return IC_ROWINGD;
		case ND_STRINGINIT:		return IC_STRINGINIT;
		case ND_COPY00:			return IC_COPY00;
		case ND_COPY01:			return IC_COPY01;
		case ND_COPY10:			return IC_COPY10;
		case ND_COPY11:			return IC_COPY11;
		case ND_COPY0ST:		return IC_COPY0ST;
		case ND_COPY1ST:		return IC_COPY1ST;
		case ND_COPY0STASSIGN:	return IC_COPY0STASS;
		case ND_COPY1STASSIGN:	return IC_COPY1STASS;
		case ND_COPYST:			return IC_COPYST;
		case ND_CREATEDIRECT:	return IC_CREATEDIRECT;
		case ND_EXITDIRECT:		return IC_EXITDIRECT;
		case ND_ABSI:			return IC_ABSI;
		case ND_ABS:			return IC_ABS;
		case ND_SQRT:			return IC_SQRT;
		case ND_EXP:			return IC_EXP;
		case ND_SIN:			return IC_SIN;
		case ND_COS:			return IC_COS;
		case ND_LOG:			return IC_LOG;
		case ND_LOG10:			return IC_LOG10;
		case ND_ASIN:			return IC_ASIN;
		case ND_RAND:			return IC_RAND;
		case ND_ROUND:			return IC_ROUND;
		case ND_STRCPY:			return IC_STRCPY;
		case ND_STRNCPY:		return IC_STRNCPY;
		case ND_STRCAT:			return IC_STRCAT;
		case ND_STRNCAT:		return IC_STRNCAT;
		case ND_STRCMP:			return IC_STRCMP;
		case ND_STRNCMP:		return IC_STRNCMP;
		case ND_STRSTR:			return IC_STRSTR;
		case ND_STRLEN:			return IC_STRLEN;
		case ND_MSG_SEND:		return IC_MSG_SEND;
		case ND_MSG_RECEIVE:	return IC_MSG_RECEIVE;
		case ND_JOIN:			return IC_JOIN;
		case ND_SLEEP:			return IC_SLEEP;
		case ND_SEMCREATE:		return IC_SEMCREATE;
		case ND_SEMWAIT:		return IC_SEMWAIT;
		case ND_SEMPOST:		return IC_SEMPOST;
		case ND_CREATE:			return IC_CREATE;
		case ND_INIT:			return IC_INIT;
		case ND_DESTROY:		return IC_DESTROY;
		case ND_EXIT:			return IC_EXIT;
		case ND_GETNUM:			return IC_GETNUM;
		case ND_UPB:			return IC_UPB;
		case ND_SEND_INT:		return IC_SEND_INT;
		case ND_SEND_FLOAT:		return IC_SEND_FLOAT;
		case ND_SEND_STRING:	return IC_SEND_STRING;
		case ND_RECEIVE_INT:	return IC_RECEIVE_INT;
		case ND_RECEIVE_FLOAT:	return IC_RECEIVE_FLOAT;
		case ND_RECEIVE_STRING:	return IC_RECEIVE_STRING;
		case ND_ASSERT:			return IC_ASSERT;
		case ND_GETID:			return IC_GETID;
		case ND_PRINTF:			return IC_PRINTF;
		case ND_PRINT:			return IC_PRINT;
		case ND_PRINTID:		return IC_PRINTID;
		case ND_REMASSIGN:		return IC_REMASS;
		case ND_SHLASSIGN:		return IC_SHLASS;
		case ND_SHRASSIGN:		return IC_SHRASS;
		case ND_ANDASSIGN:		return IC_ANDASS;
		case ND_XORASSIGN:		return IC_EXORASS;
		case ND_ORASSIGN:		return IC_ORASS;
		case ND_ASSIGN:			return IC_ASS;
		case ND_ADDASSIGN:		return IC_PLUSASS;
		case ND_SUBASSIGN:		return IC_MINUSASS;
		case ND_MULASSIGN:		return IC_MULTASS;
		case ND_DIVASSIGN:		return IC_DIVASS;
		case ND_REMASSIGNAT:	return IC_REMASSAT;
		case ND_SHLASSIGNAT:	return IC_SHLASSAT;
		case ND_SHRASSIGNAT:	return IC_SHRASSAT;
		case ND_ANDASSIGNAT:	return IC_ANDASSAT;
		case ND_XORASSIGNAT:	return IC_EXORASSAT;
		case ND_ORASSIGNAT:		return IC_ORASSAT;
		case ND_ASSIGNAT:		return IC_ASSAT;
		case ND_ADDASSIGNAT:	return IC_PLUSASSAT;
		case ND_SUBASSIGNAT:	return IC_MINUSASSAT;
		case ND_MULASSIGNAT:	return IC_MULTASSAT;
		case ND_DIVASSIGNAT:	return IC_DIVASSAT;
		case ND_REM:			return IC_REM;
		case ND_SHL:			return IC_SHL;
		case ND_SHR:			return IC_SHR;
		case ND_AND:			return IC_AND;
		case ND_XOR:			return IC_EXOR;
		case ND_OR:				return IC_OR;
		case ND_LOGAND:			return IC_LOGAND;
		case ND_LOGOR:			return IC_LOGOR;
		case ND_EQ:				return IC_EQEQ;
		case ND_NE:				return IC_NOTEQ;
		case ND_LT:				return IC_LT;
		case ND_GT:				return IC_GT;
		case ND_LE:				return IC_LE;
		case ND_GE:				return IC_GE;
		case ND_ADD:			return IC_PLUS;
		case ND_SUB:			return IC_MINUS;
		case ND_MUL:			return IC_MULT;
		case ND_DIV:			return IC_DIV;
		case ND_POSTINC:		return IC_POSTINC;
		case ND_POSTDEC:		return IC_POSTDEC;
		case ND_PREINC:			return IC_INC;
		case ND_PREDEC:			return IC_DEC;
		case ND_POSTINCAT:		return IC_POSTINCAT;
		case ND_POSTDECAT:		return IC_POSTDECAT;
		case ND_PREINCAT:		return IC_INCAT;
		case ND_PREDECAT:		return IC_DECAT;
		case ND_UNMINUS:		return IC_UNMINUS;
		case ND_NOT:			return IC_NOT;
		case ND_LOGNOT:			return IC_LOGNOT;
		case ND_ASSIGNR:		return IC_ASSR;
		case ND_ADDASSIGNR:		return IC_PLUSASSR;
		case ND_SUBASSIGNR:		return IC_MINUSASSR;
		case ND_MULASSIGNR:		return IC_MULTASSR;
		case ND_DIVASSIGNR:		return IC_DIVASSR;
		case ND_ASSIGNATR:		return IC_ASSATR;
		case ND_ADDASSIGNATR:	return IC_PLUSASSATR;
		case ND_SUBASSIGNATR:	return IC_MINUSASSATR;
		case ND_MULASSIGNATR:	return IC_MULTASSATR;
		case ND_DIVASSIGNATR:	return IC_DIVASSATR;
		case ND_EQR:			return IC_EQEQR;
		case ND_NER:			return IC_NOTEQR;
		case ND_LTR:			return IC_LTR;
		case ND_GTR:			return IC_GTR;
		case ND_LER:			return IC_LER;
		case ND_GER:			return IC_GER;
		case ND_ADDR:			return IC_PLUSR;
		case ND_SUBR:			return IC_MINUSR;
		case ND_MULR:			return IC_MULTR;
		case ND_DIVR:			return IC_DIVR;
		case ND_POSTINCR:		return IC_POSTINCR;
		case ND_POSTDECR:		return IC_POSTDECR;
		case ND_PREINCR:		return IC_INCR;
		case ND_PREDECR:		return IC_DECR;
		case ND_POSTINCATR:		return IC_POSTINCATR;
		case ND_POSTDECATR:		return IC_POSTDECATR;
		case ND_PREINCATR:		return IC_INCATR;
		case ND_PREDECATR:		return IC_DECATR;
		case ND_UNMINUSR:		return IC_UNMINUSR;
		case ND_REMASSIGNV:		return IC_REMASSV;
		case ND_SHLASSIGNV:		return IC_SHLASSV;
		case ND_SHRASSIGNV:		return IC_SHRASSV;
		case ND_ANDASSIGNV:		return IC_ANDASSV;
		case ND_XORASSIGNV:		return IC_EXORASSV;
		case ND_ORASSIGNV:		return IC_ORASSV;
		case ND_ASSIGNV:		return IC_ASSV;
		case ND_ADDASSIGNV:		return IC_PLUSASSV;
		case ND_SUBASSIGNV:		return IC_MINUSASSV;
		case ND_MULASSIGNV:		return IC_MULTASSV;
		case ND_DIVASSIGNV:		return IC_DIVASSV;
		case ND_REMASSIGNATV:	return IC_REMASSATV;
		case ND_SHLASSIGNATV:	return IC_SHLASSATV;
		case ND_SHRASSIGNATV:	return IC_SHRASSATV;
		case ND_ANDASSIGNATV:	return IC_ANDASSATV;
		case ND_XORASSIGNATV:	return IC_EXORASSATV;
		case ND_ORASSIGNATV:	return IC_ORASSATV;
		case ND_ASSIGNATV:		return IC_ASSATV;
		case ND_ADDASSIGNATV:	return IC_PLUSASSATV;
		case ND_SUBASSIGNATV:	return IC_MINUSASSATV;
		case ND_MULASSIGNATV:	return IC_MULTASSATV;
		case ND_DIVASSIGNATV:	return IC_DIVASSATV;
		case ND_ASSIGNRV:		return IC_ASSRV;
		case ND_ADDASSIGNRV:	return IC_PLUSASSRV;
		case ND_SUBASSIGNRV:	return IC_MINUSASSRV;
		case ND_MULASSIGNRV:	return IC_MULTASSRV;
		case ND_DIVASSIGNRV:	return IC_DIVASSRV;
		case ND_ASSIGNATRV:		return IC_ASSATRV;
		case ND_ADDASSIGNATRV:	return IC_PLUSASSATRV;
		case ND_SUBASSIGNATRV:	return IC_MINUSASSATRV;
		case ND_MULASSIGNATRV:	return IC_MULTASSATRV;
		case ND_DIVASSIGNATRV:	return IC_DIVASSATRV;
		case ND_POSTINCV:		return IC_POSTINCV;
		case ND_POSTDECV:		return IC_POSTDECV;
		case ND_PREINCV:		return IC_INCV;
		case ND_PREDECV:		return IC_DECV;
		case ND_POSTINCATV:		return IC_POSTINCATV;
		case ND_POSTDECATV:		return IC_POSTDECATV;
		case ND_PREINCATV:		return IC_INCATV;
		case ND_PREDECATV:		return IC_DECATV;
		case ND_POSTINCRV:		return IC_POSTINCRV;
		case ND_POSTDECRV:		return IC_POSTDECRV;
		case ND_PREINCRV:		return IC_INCRV;
		case ND_PREDECRV:		return IC_DECRV;
		case ND_POSTINCATRV:	return IC_POSTINCATRV;
		case ND_POSTDECATRV:	return IC_POSTDECATRV;
		case ND_PREINCATRV:		return IC_INCATRV;
		case ND_PREDECATRV:		return IC_DECATRV;
		default:				return 0;
	}
}

node_t token_to_node(const token_t token)
{
	switch (token)
	{
		case TOK_TILDE:					return ND_NOT;
		case TOK_PLUS:					return ND_ADD;
		case TOK_MINUS:					return ND_SUB;
		case TOK_STAR:					return ND_MUL;
		case TOK_SLASH:					return ND_DIV;
		case TOK_PERCENT:				return ND_REM;
		case TOK_EXCLAIM:				return ND_LOGNOT;
		case TOK_CARET:					return ND_XOR;
		case TOK_PIPE:					return ND_OR;
		case TOK_AMP:					return ND_AND;
		case TOK_EQUAL:					return ND_ASSIGN;
		case TOK_LESS:					return ND_LT;
		case TOK_GREATER:				return ND_GT;
		case TOK_PLUSEQUAL:				return ND_ADDASSIGN;
		case TOK_PLUSPLUS:				return ND_PREINC;
		case TOK_MINUSEQUAL:			return ND_SUBASSIGN;
		case TOK_MINUSMINUS:			return ND_PREDEC;
		case TOK_STAREQUAL:				return ND_MULASSIGN;
		case TOK_SLASHEQUAL:			return ND_DIVASSIGN;
		case TOK_PERCENTEQUAL:			return ND_REMASSIGN;
		case TOK_EXCLAIMEQUAL:			return ND_NE;
		case TOK_CARETEQUAL:			return ND_XORASSIGN;
		case TOK_PIPEEQUAL:				return ND_ORASSIGN;
		case TOK_PIPEPIPE:				return ND_LOGOR;
		case TOK_AMPEQUAL:				return ND_ANDASSIGN;
		case TOK_AMPAMP:				return ND_LOGAND;
		case TOK_EQUALEQUAL:			return ND_EQ;
		case TOK_LESSEQUAL:				return ND_LE;
		case TOK_LESSLESS:				return ND_SHL;
		case TOK_GREATEREQUAL:			return ND_GE;
		case TOK_GREATERGREATER:		return ND_SHR;
		case TOK_LESSLESSEQUAL:			return ND_SHLASSIGN;
		case TOK_GREATERGREATEREQUAL:	return ND_SHRASSIGN;

		case TOK_ABS:					return ND_ABS;
		case TOK_SQRT:					return ND_SQRT;
		case TOK_EXP:					return ND_EXP;
		case TOK_SIN:					return ND_SIN;
		case TOK_COS:					return ND_COS;
		case TOK_LOG:					return ND_LOG;
		case TOK_LOG10:					return ND_LOG10;
		case TOK_ASIN:					return ND_ASIN;
		case TOK_RAND:					return ND_RAND;
		case TOK_ROUND:					return ND_ROUND;
		case TOK_STRCPY:				return ND_STRCPY;
		case TOK_STRNCPY:				return ND_STRNCPY;
		case TOK_STRCAT:				return ND_STRCAT;
		case TOK_STRNCAT:				return ND_STRNCAT;
		case TOK_STRCMP:				return ND_STRCMP;
		case TOK_STRNCMP:				return ND_STRNCMP;
		case TOK_STRSTR:				return ND_STRSTR;
		case TOK_STRLEN:				return ND_STRLEN;
		case TOK_ASSERT:				return ND_ASSERT;
		case TOK_UPB:					return ND_UPB;
		case TOK_CREATEDIRECT:			return ND_CREATEDIRECT;
		case TOK_EXITDIRECT:			return ND_EXITDIRECT;
		case TOK_MSG_SEND:				return ND_MSG_SEND;
		case TOK_MSG_RECEIVE:			return ND_MSG_RECEIVE;
		case TOK_JOIN:					return ND_JOIN;
		case TOK_SLEEP:					return ND_SLEEP;
		case TOK_SEMCREATE:				return ND_SEMCREATE;
		case TOK_SEMWAIT:				return ND_SEMWAIT;
		case TOK_SEMPOST:				return ND_SEMPOST;
		case TOK_CREATE:				return ND_CREATE;
		case TOK_INIT:					return ND_INIT;
		case TOK_DESTROY:				return ND_DESTROY;
		case TOK_EXIT:					return ND_EXIT;
		case TOK_GETNUM:				return ND_GETNUM;
		case TOK_SEND_INT:				return ND_SEND_INT;
		case TOK_SEND_FLOAT:			return ND_SEND_FLOAT;
		case TOK_SEND_STRING:			return ND_SEND_STRING;
		case TOK_RECEIVE_INT:			return ND_RECEIVE_INT;
		case TOK_RECEIVE_FLOAT:			return ND_RECEIVE_FLOAT;
		case TOK_RECEIVE_STRING:		return ND_RECEIVE_STRING;
		default:						return 0;
	}
}

node_t node_at_operator(const node_t node)
{
	switch (node)
	{
		case ND_PREINC:		return ND_PREINCAT;
		case ND_PREDEC:		return ND_PREDECAT;
		case ND_POSTINC:	return ND_POSTINCAT;
		case ND_POSTDEC:	return ND_POSTDECAT;

		case ND_REMASSIGN:	return ND_REMASSIGNAT;
		case ND_SHLASSIGN:	return ND_SHLASSIGNAT;
		case ND_SHRASSIGN:	return ND_SHRASSIGNAT;
		case ND_ANDASSIGN:	return ND_ANDASSIGNAT;
		case ND_XORASSIGN:	return ND_XORASSIGNAT;
		case ND_ORASSIGN:	return ND_ORASSIGNAT;
		case ND_ASSIGN:		return ND_ASSIGNAT;
		case ND_ADDASSIGN:	return ND_ADDASSIGNAT;
		case ND_SUBASSIGN:	return ND_SUBASSIGNAT;
		case ND_MULASSIGN:	return ND_MULASSIGNAT;
		case ND_DIVASSIGN:	return ND_DIVASSIGNAT;

		default: 			return 0;
	}
}

node_t node_void_operator(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIVASSIGNAT)
		|| (node >= ND_POSTINC && node <= ND_PREDECAT)
		|| (node >= ND_ASSIGNR && node <= ND_DIVASSIGNATR)
		|| (node >= ND_POSTINCR && node <= ND_PREDECATR))
	{
		return node + 200;
	}
	else
	{
		return node;
	}
}

node_t node_float_operator(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIVASSIGN)
		|| (node >= ND_ASSIGNAT && node <= ND_DIVASSIGNAT)
		|| (node >= ND_EQ && node <= ND_UNMINUS))
	{
		return node + 50;
	}
	else
	{
		return node;
	}
}

int node_is_assignment_operator(const node_t op)
{
	if ((op >= ND_REMASSIGN && op <= ND_DIVASSIGN) || (op >= ND_REMASSIGNV && op <= ND_DIVASSIGNV)
		|| (op >= ND_ASSIGNR && op <= ND_DIVASSIGNR) || (op >= ND_ASSIGNRV && op <= ND_DIVASSIGNRV)
		|| (op >= ND_POSTINC && op <= ND_PREDEC) || (op >= ND_POSTINCV && op <= ND_PREDECV)
		|| (op >= ND_POSTINCR && op <= ND_PREDECR) || (op >= ND_POSTINCRV && op <= ND_PREDECRV))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
