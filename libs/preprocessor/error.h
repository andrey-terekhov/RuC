/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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

#pragma once

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Errors codes */
enum ERROR
{
    after_ident_must_be_space = 365,
    must_be_endif,
    dont_elif,
    preproces_words_not_exist,
    not_enough_param,
    functionid_begins_with_letters,
    after_functionid_must_be_comma,
    stalpe,
    before_endif,
    repeat_ident,
    not_end_fail_define,
    scope_not_close,
    after_preproces_words_must_be_space,
    ident_begins_with_letters,
    ident_not_exist,
    functions_cannot_be_changed,
    after_eval_must_be_ckob,
    too_many_nuber,
    not_arithmetic_operations,
    not_logical_operations,
    comm_not_ended,
    not_enough_param2,
    must_be_digit_after_exp1,
    not_macro,
    incorrect_arithmetic_expression,
    third_party_symbol,
    in_eval_must_end_parenthesis,
    must_end_quote,
    must_start_quote,
    macro_does_not_exist,
    must_end_endw,
    include_file_not_found,
    source_file_not_found,
    no_string_ending,
};


/**
 *	Emit an error for some problem
 *
 *	@param	num			Error code
 *	@param	path		Current file path
 *	@param	code		Error line in current file
 *	@param	line		Error line number
 *	@param	position	Error position in line
 */
void macro_error(const int num, const char *const path, const char *const code, const size_t line, size_t position);

/**
 *	Emit an error for some problem
 *
 *	@param	tag		Message location
 *	@param	num		Error code
 */
void macro_system_error(const char *const tag, const int num);

#ifdef __cplusplus
} /* extern "C" */
#endif
