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
	needless_elif,
	preprocess_word_not_exists,
	more_than_enough_arguments,
	functionid_must_begins_with_letter,
	after_functionid_must_be_comma,
	stalpe,
	extra_endif,
	ident_is_repeated,
	define_at_the_end,
	invalid_parenthesis_entry,
	after_preproces_words_must_be_space,
	ident_must_begins_with_letter,
	ident_not_exists,
	functions_cannot_be_changed,
	after_eval_must_be_parenthesis,
	too_many_nuber,
	arithmetic_operations_must_be_in_eval,
	logical_operations_are_prohibited_in_eval,
	comm_not_ended,
	not_enough_arguments,
	after_exp_must_be_digit,
	ident_not_macro,
	incorrect_arithmetic_expression,
	third_party_symbol,
	eval_must_end_with_parenthesis,
	file_name_must_end_with_quote,
	file_name_must_start_with_quote,
	macro_not_exists,
	cicle_must_end_with_endw,
	included_file_not_found,
	source_file_not_found,
	must_be_string_ending, 
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
