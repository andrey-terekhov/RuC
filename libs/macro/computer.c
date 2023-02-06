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
#include "error.h"


static const size_t MAX_EXPRESSION_DAPTH = 64;


/**
 *	Emit an error from computer
 *
 *	@param	prs			Сomputer structure
 *	@param	num			Error code
 */
static void computer_error(computer *const comp, const size_t pos, error_t num, ...)
{
	if (in_is_file(comp->io))
	{
		const size_t origin = in_get_position(comp->io);
		in_set_position(comp->io, pos);
		loc_search_from(&comp->loc);
		in_set_position(comp->io, origin);
	}

	if (loc_is_correct(&comp->loc))
	{
		va_list args;
		va_start(args, num);

		macro_verror(&comp->loc, num, args);
		comp->loc = loc_copy(NULL);

		va_end(args);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


computer computer_create(location *const loc, universal_io *const io)
{
	return (computer) { .loc = loc_copy(loc), .io = io
		, .numbers = stack_create(MAX_EXPRESSION_DAPTH)
		, .operators = stack_create(MAX_EXPRESSION_DAPTH) };
}

int computer_push_token(computer *const comp, const size_t pos, const token_t tk)
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
			printf("%% ");
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

int computer_push_number(computer *const comp, const size_t pos, const item_t num)
{
	printf("%" PRIitem " ", num);
	return 0;
}

int computer_push_const(computer *const comp, const size_t pos, const char32_t ch, const char *const name)
{
	printf("%s ", name);
	return 0;
}


bool computer_is_correct(const computer *const comp)
{
	return comp != NULL && stack_is_correct(&comp->numbers) && stack_is_correct(&comp->operators);
}

item_t computer_pop_result(computer *const comp)
{
	return 0;
}

int computer_clear(computer *const comp)
{
	if (!computer_is_correct(comp))
	{
		return -1;
	}

	stack_clear(&comp->numbers);
	stack_clear(&comp->operators);

	printf("\n");
	return 0;
}
