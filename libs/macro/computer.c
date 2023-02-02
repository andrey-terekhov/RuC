/*
 *	Copyright 2023 Andrey Terekhov, Victor Y. Fadeev
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


#include "computer.h"


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


computer computer_create(location *const loc);

int computer_push_token(computer *const comp, location *const loc, const token_t tk)
{
	switch(tk)
	{
		case TK_COMPL:
			printf("~ ");
			break;
		case TK_NOT:
			printf("! ");
			break;
		case TK_MULT:
			printf("* ");
			break;
		case TK_DIV:
			printf("/ ");
			break;
		case TK_MOD:
			printf("% ");
			break;
		case TK_ADD:
			printf("+ ");
			break;
		case TK_SUB:
			printf("- ");
			break;
		case TK_L_SHIFT:
			printf("<< ");
			break;
		case TK_R_SHIFT:
			printf(">> ");
			break;
		case TK_LESS:
			printf("< ");
			break;
		case TK_GREATER:
			printf("> ");
			break;
		case TK_LESS_EQ:
			printf("<= ");
			break;
		case TK_GREATER_EQ:
			printf(">= ");
			break;
		case TK_EQ:
			printf("== ");
			break;
		case TK_NOT_EQ:
			printf("!= ");
			break;
		case TK_BIT_AND:
			printf("& ");
			break;
		case TK_XOR:
			printf("^ ");
			break;
		case TK_BIT_OR:
			printf("| ");
			break;
		case TK_AND:
			printf("&& ");
			break;
		case TK_OR:
			printf("|| ");
			break;
	}

	return 0;
}

int computer_push_number(computer *const comp, location *const loc, const item_t num)
{
	printf("%" PRIitem " ", num);
	return 0;
}

int computer_push_const(computer *const comp, location *const loc, const char32_t ch, const char *const name)
{
	printf("%s ", name);
	return 0;
}


bool computer_is_correct(const computer *const comp);
int computer_clear(computer *const comp);
