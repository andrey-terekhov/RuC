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

#include "token.h"


/*
 *   __     __   __     ______   ______     ______     ______   ______     ______     ______
 *  /\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *  \ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *   \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *    \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


location token_get_location(const token *const tk)
{
	return tk->loc;
}

token_t token_get_kind(const token *const tk)
{
	return tk->kind;
}

bool token_is(const token *const tk, const token_t kind)
{
	return tk->kind == kind;
}

bool token_is_not(const token *const tk, const token_t kind)
{
	return tk->kind != kind;
}


token token_identifier(const location loc, const size_t name)
{
	return (token){ .loc = loc, .kind = TK_IDENTIFIER, .data.ident_repr = name };
}

size_t token_get_ident_name(const token *const tk)
{
	assert(tk->kind == TK_IDENTIFIER);
	return tk->data.ident_repr;
}


token token_char_literal(const location loc, const char32_t value)
{
	return (token){ .loc = loc, .kind = TK_CHAR_LITERAL, .data.char_value = value };
}

char32_t token_get_char_value(const token *const tk)
{
	assert(tk->kind == TK_CHAR_LITERAL);
	return tk->data.char_value;
}


token token_int_literal(const location loc, const uint64_t value)
{
	return (token){ .loc = loc, .kind = TK_INT_LITERAL, .data.int_value = value };
}

uint64_t token_get_int_value(const token *const tk)
{
	assert(tk->kind == TK_INT_LITERAL);
	return tk->data.int_value;
}


token token_float_literal(const location loc, const double value)
{
	return (token){ .loc = loc, .kind = TK_FLOAT_LITERAL, .data.float_value = value };
}

double token_get_float_value(const token *const tk)
{
	assert(tk->kind == TK_FLOAT_LITERAL);
	return tk->data.float_value;
}


token token_string_literal(const location loc, const size_t string_num)
{
	return (token){ .loc = loc, .kind = TK_STRING_LITERAL, .data.string_num = string_num };
}

size_t token_get_string_num(const token *const tk)
{
	assert(tk->kind == TK_STRING_LITERAL);
	return tk->data.string_num;
}


token token_eof(void)
{
	return (token){ .loc = { SIZE_MAX, SIZE_MAX }, .kind = TK_EOF };
}

token token_keyword(const location loc, const token_t kind)
{
	return (token){ .loc = loc, .kind = kind };
}

token token_punctuator(const location loc, const token_t kind)
{
	return (token){ .loc = loc, .kind = kind };
}
