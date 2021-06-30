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

#include "operations.h"
#include "errors.h"


binary_t token_to_binary(const token_t token)
{
	switch (token)
	{
		case TK_PLUS:					return BIN_ADD;
		case TK_MINUS:					return BIN_SUB;
		case TK_STAR:					return BIN_MUL;
		case TK_SLASH:					return BIN_DIV;
		case TK_PERCENT:				return BIN_REM;
		case TK_CARET:					return BIN_XOR;
		case TK_PIPE:					return BIN_OR;
		case TK_AMP:					return BIN_AND;
		case TK_EQUAL:					return BIN_ASSIGN;
		case TK_LESS:					return BIN_LT;
		case TK_GREATER:				return BIN_GT;
		case TK_PLUS_EQUAL:				return BIN_ADD_ASSIGN;
		case TK_MINUS_EQUAL:			return BIN_SUB_ASSIGN;
		case TK_STAR_EQUAL:				return BIN_MUL_ASSIGN;
		case TK_SLASH_EQUAL:			return BIN_DIV_ASSIGN;
		case TK_PERCENT_EQUAL:			return BIN_REM_ASSIGN;
		case TK_EXCLAIM_EQUAL:			return BIN_NE;
		case TK_CARET_EQUAL:			return BIN_XOR_ASSIGN;
		case TK_PIPE_EQUAL:				return BIN_OR_ASSIGN;
		case TK_PIPE_PIPE:				return BIN_LOG_OR;
		case TK_AMP_EQUAL:				return BIN_AND_ASSIGN;
		case TK_AMP_AMP:				return BIN_LOG_AND;
		case TK_EQUAL_EQUAL:			return BIN_EQ;
		case TK_LESS_EQUAL:				return BIN_LE;
		case TK_LESS_LESS:				return BIN_SHL;
		case TK_GREATER_EQUAL:			return BIN_GE;
		case TK_GREATER_GREATER:		return BIN_SHR;
		case TK_LESS_LESS_EQUAL:		return BIN_SHL_ASSIGN;
		case TK_GREATER_GREATER_EQUAL:	return BIN_SHR_ASSIGN;
		case TK_COMMA:					return BIN_COMMA;

		default:
			// Unreachable
			return 0;
	}
}

unary_t token_to_unary(const token_t token)
{
	switch (token)
	{
		case TK_PLUS_PLUS:		return UN_PREINC;
		case TK_MINUS_MINUS:	return UN_PREDEC;
		case TK_AMP:			return UN_ADDRESS;
		case TK_STAR:			return UN_INDIRECTION;
		case TK_PLUS:			return UN_PLUS;
		case TK_MINUS:			return UN_MINUS;
		case TK_TILDE:			return UN_NOT;
		case TK_EXCLAIM:		return UN_LOGNOT;

		default:
			// Unreachable
			return 0;
	}
}

bool operation_is_assignment(const binary_t operator)
{
	switch (operator)
	{
		case BIN_ASSIGN:
		case BIN_ADD_ASSIGN:
		case BIN_SUB_ASSIGN:
		case BIN_MUL_ASSIGN:
		case BIN_DIV_ASSIGN:
		case BIN_REM_ASSIGN:
		case BIN_XOR_ASSIGN:
		case BIN_OR_ASSIGN:
		case BIN_AND_ASSIGN:
		case BIN_SHL_ASSIGN:
		case BIN_SHR_ASSIGN:
			return true;

		default:
			return false;
	}
}
