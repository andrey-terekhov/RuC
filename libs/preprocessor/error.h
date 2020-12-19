/*
 *	Copyright  Andrey Terekhov, Egor Anikin
 *
 *	Licensed under the Apache License, Version . (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *http://www.apache.org/licenses/LICENSE-.
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
	ident_begins_with_s,//
	must_be_endif,
	dont_elif,
	preproces_words_not_exist,//
	not_enough_param,
	functionid_begins_with_letters,
	after_functionid_must_be_comma,
	stalpe,
	before_endif,
	repeat_ident,
	not_end_fail_define,
	scob_not_clous,
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
};


/**
 *	Emit an error for some problem
 *
 *	@param	num			Error code
 *	@param	path		Path current file
 *	@param	line		Error line number
 *	@param	code		Error line
 *	@param	position	Error position in line
 */
void macro_error(const int num, const char *const path, const size_t line, const char *const code, size_t position);

#ifdef __cplusplus
} /* extern "C" */
#endif
