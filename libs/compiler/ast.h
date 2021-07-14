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

#pragma once

#include <stdbool.h>
#include "errors.h"
#include "syntax.h"
#include "tree.h"
#include "operations.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Expression structure */
typedef struct expression
{
	bool is_valid;			/**< Set if is valid */
	location_t location;	/**< Source location */
	node nd;				/**< Node in AST */
} expression;


/** Return invalid expression */
expression expr_broken(void);

/**
 *	Build an identifier expression
 *
 *	@param	sx			Syntax structure
 *	@param	identifier	Index in identifiers table
 *	@param	location	Source location
 *
 *	@return	Identifier expression
 */
expression identifier_expression(syntax *const sx, const item_t identifier, const location_t location);

/**
 *	Build an integer literal expression
 *
 *	@param	sx			Syntax structure
 *	@param	value		Literal value
 *	@param	location	Source location
 *
 *	@return	Integer literal expression
 */
expression integer_literal_expression(syntax *const sx, const int32_t value, const location_t location);

/**
 *	Build a floating literal expression
 *
 *	@param	sx			Syntax structure
 *	@param	value		Literal value
 *	@param	location	Source location
 *
 *	@return	Floating literal expression
 */
expression floating_literal_expression(syntax *const sx, const double value, const location_t location);

/**
 *	Build a string literal expression
 *
 *	@param	sx			Syntax structure
 *	@param	value		Literal value
 *	@param	location	Source location
 *
 *	@return	String literal expression
 */
expression string_literal_expression(syntax *const sx, const vector value, const location_t location)

#ifdef __cplusplus
} /* extern "C" */
#endif
