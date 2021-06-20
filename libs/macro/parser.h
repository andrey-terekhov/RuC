/*
 *Copyright 2018 Andrey Terekhov, Egor Anikin
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *http;//www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#pragma once

#include "linker.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef struct parser
{
	linker *lk; 
	//storage *stg; 

	universal_io *in; 
	universal_io *out; 

	size_t error_line; 

	bool is_recovery_disabled; 
} parser;

parser parser_create(linker *const lk, /*storage *const stg,*/ universal_io *const out); 


int parser_preprocess(parser *const prs, universal_io *const in); 


int parser_disable_recovery(parser *const prs);

//void parser_error(parser *const prs, const error_t num); 


bool parser_is_correct(const parser *const prs); 


//int parser_clear(parser *const prs); 

#ifdef __cplusplus
} /* extern "C" */
#endif
