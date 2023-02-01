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
		case TK_NOT:
			printf("! ");
		case TK_MULT:
			printf("* ");
		case TK_DIV:
			printf("/ ");
		case TK_MOD:
			printf("% ");
		case TK_ADD:
			printf("+ ");
		case TK_SUB:
			printf("- ");
		case TK_L_SHIFT:
			printf("<< ");
		case TK_R_SHIFT:
			printf(">> ");
		case TK_LESS:
			printf("< ");
		case TK_GREATER:
			printf("> ");
		case TK_LESS_EQ:
			printf("<= ");
		case TK_GREATER_EQ:
			printf(">= ");
		case TK_EQ:
			printf("== ");
		case TK_NOT_EQ:
			printf("!= ");
		case TK_BIT_AND:
			printf("& ");
		case TK_XOR:
			printf("^ ");
		case TK_BIT_OR:
			printf("| ");
		case TK_AND:
			printf("&& ");
		case TK_OR:
			printf("|| ");
	}

	return 0;
}

int computer_push_number(computer *const comp, location *const loc, const item_t num)
{
	printf("%" PRIitem " ", num);
	return 0;
}

int computer_push_const(computer *const comp, location *const loc, const char32_t ch)
{
	char buffer[8];
	utf8_to_string(buffer, ch);
	printf("'%s' ", buffer);
	return 0;
}


bool computer_is_correct(const computer *const comp);
int computer_clear(computer *const comp);
