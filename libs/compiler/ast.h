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
	location_t loc;			/**< Source location */
	node nd;				/**< Node in AST */
} expression;


/** Return invalid expression */
expression invalid_expression(void);

/**
 *	Build an identifier expression
 *
 *	@param	sx				Syntax structure
 *	@param	identifier		Index in identifiers table
 *	@param	loc				Source location
 *
 *	@return	Identifier expression
 */
expression identifier_expression(syntax *const sx, const item_t identifier, const location_t loc);

/**
 *	Build an integer literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Integer literal expression
 */
expression integer_literal_expression(syntax *const sx, const int32_t value, const location_t loc);

/**
 *	Build a floating literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Floating literal expression
 */
expression floating_literal_expression(syntax *const sx, const double value, const location_t loc);

/**
 *	Build a string literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	String literal expression
 */
expression string_literal_expression(syntax *const sx, const vector value, const location_t loc);

/**
 *	Build a subscripting expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of subscripting expression
 *	@param	index			Second operand of subscripting expression
 *	@param	l_square_loc	Left square bracket location
 *	@param	r_square_loc	Right square bracket location
 *
 *	@return	Subscripting expression
 */
expression subscripting_expression(syntax *const sx, const expression base, const expression index
								   , const location_t l_square_loc, const location_t r_square_loc);

/**
 *	Build a member expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of member expression
 *	@param	is_arrow		Set if operator is arrow
 *	@param	operator_loc	Operator source location
 *	@param	identifier		Second operand of member expression
 *	@param	identifier_loc	Identifier source location
 *
 *	@return	Member expression
 */
expression member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t identifier
							 , const location_t operator_loc, const location_t identifier_loc);

#ifdef __cplusplus
} /* extern "C" */
#endif
