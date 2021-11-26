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


extern location token_get_location(const token *const tk);
extern token_t token_get_kind(const token *const tk);
extern bool token_is(const token *const tk, const token_t kind);
extern bool token_is_not(const token *const tk, const token_t kind);

extern token token_identifier(const location loc, const size_t name);
extern size_t token_get_ident_name(const token *const tk);

extern token token_char_literal(const location loc, const char32_t value);
char32_t token_get_char_value(const token *const tk);

extern token token_int_literal(const location loc, const int64_t value);
extern int64_t token_get_int_value(const token *const tk);

extern token token_float_literal(const location loc, const double value);
extern double token_get_float_value(const token *const tk);

extern token token_string_literal(const location loc, const size_t string_num);
extern size_t token_get_string_num(const token *const tk);

extern token token_eof();
extern token token_keyword(const location loc, const token_t kind);
extern token token_punctuator(const location loc, const token_t kind);
