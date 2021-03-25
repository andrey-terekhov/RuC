/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "map.h"
#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct node node;

/** Global vars definition */
typedef struct syntax
{
	vector predef;				/**< Predefined functions table */
	vector functions;			/**< Functions table */

	vector tree;				/**< Tree table */

	vector identifiers;			/**< Identifiers table */
	size_t cur_id;				/**< Start of current scope in identifiers table */

	vector modes;				/**< Modes table */
	size_t start_mode;			/**< Start of last record in modetab */

	map representations;		/**< Representations table */

	item_t max_displ;			/**< Max displacement */
	item_t max_displg;			/**< Max displacement */

	item_t displ;				/**< Stack displacement in current scope */
	item_t lg;					/**< Displacement from l (+1) or g (-1) */

	size_t procd;				/**< Process management daemon */
	size_t ref_main;			/**< Main function reference */
} syntax;


/**
 *	Create Syntax structure
 *
 *	@return	Syntax structure
 */
syntax sx_create();

/**
 *	Check if syntax structure is correct
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int sx_is_correct(syntax *const sx);

/**
 *	Free allocated memory
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int sx_clear(syntax *const sx);


/**
 *	Add a new record to functions table
 *
 *	@param	sx			Syntax structure
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_add(syntax *const sx, const item_t ref);

/**
 *	Set function start reference by index in functions table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_set(syntax *const sx, const size_t index, const item_t ref);

/**
 *	Get an item from functions table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *
 *	@return	Item by index from functions table, @c ITEM_MAX on failure
 */
item_t func_get(const syntax *const sx, const size_t index);

/**
 *	Reserve one element in functions table
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Previous functions size
 */
size_t func_reserve(syntax *const sx);


/**
 *	Add new item to identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	repr		New identifier index in representations table
 *	@param	type		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	mode		New identifier mode
 *	@param	func_def	@c 0 for function without args,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for variables
 *
 *	@return	Index of the last item in identifiers table,
 *			@c SIZE_MAX @c - @c 1 on redeclaration
 *			@c SIZE_MAX on redefinition of main
 */
size_t ident_add(syntax *const sx, const size_t repr, const item_t type, const item_t mode, const int func_def);

/**
 *	Get index of previous declaration from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Index of previous declaration in identifiers table, @c ITEM_MAX on failure
 */
item_t ident_get_prev(const syntax *const sx, const size_t index);

/**
 *	Get item representation from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier index in representations table, @c ITEM_MAX on failure
 */
item_t ident_get_repr(const syntax *const sx, const size_t index);

/**
 *	Get item mode from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier mode, @c ITEM_MAX on failure
 */
item_t ident_get_mode(const syntax *const sx, const size_t index);

/**
 *	Get item displacement from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier displacement, @c ITEM_MAX on failure
 */
item_t ident_get_displ(const syntax *const sx, const size_t index);

/**
 *	Set identifier representation by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	repr		Index of record in representation table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_repr(syntax *const sx, const size_t index, const item_t repr);

/**
 *	Set identifier mode by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	mode		Identifier mode
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_mode(syntax *const sx, const size_t index, const item_t mode);

/**
 *	Set identifier displacement by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	displ		Identifier displacement
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_displ(syntax *const sx, const size_t index, const item_t displ);


/**
 *	Get mode size
 *	@note	Also used in codegen
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Standard type or index of the modes table
 *
 *	@return	Mode size
 */
size_t size_of(const syntax *const sx, const item_t mode);

/**
 *	Add a new record to modes table
 *
 *	@param	sx			Syntax structure
 *	@param	record		Pointer to the new record
 *	@param	size		Size of the new record
 *
 *	@return	Index of the new record in modes table, @c SIZE_MAX on failure
 */
size_t mode_add(syntax *const sx, const item_t *const record, const size_t size);

/**
 *	Get an item from modes table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in modes table
 *
 *	@return	Item by index from modes table, @c ITEM_MAX on failure
 */
item_t mode_get(const syntax *const sx, const size_t index);


/**
 *	Add a new record to representations table or return existing
 *
 *	@param	sx			Syntax structure
 *	@param	spelling	Unique UTF-8 string key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t repr_reserve(syntax *const sx, const char32_t *const spelling);

/**
 *	Get identifier name from representations table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *
 *	@return	Pointer to name of identifier
 */
const char *repr_get_name(const syntax *const sx, const size_t index);

/**
 *	Get a representation reference from table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *
 *	@return	Reference by index from representations table, @c ITEM_MAX on failure
 */
item_t repr_get_reference(const syntax *const sx, const size_t index);

/**
 *	Set representation reference by index in table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *	@param	ref			Reference to identifiers table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int repr_set_reference(syntax *const sx, const size_t index, const item_t ref);


/**
 *	Enter block scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Variable to save previous stack displacement
 *	@param	lg			Variable to save previous value of lg
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_block_enter(syntax *const sx, item_t *const displ, item_t *const lg);

/**
 *	Exit block scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Stack displacement at the start of the scope
 *	@param	lg			Previous value of lg
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_block_exit(syntax *const sx, const item_t displ, const item_t lg);

/**
 *	Enter function scope
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Previous stack displacement, @c ITEM_MAX on failure
 */
item_t scope_func_enter(syntax *const sx);

/**
 *	Exit function scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Stack displacement at the start of the scope
 *
 *	@return	Max displacement of the function, @c ITEM_MAX on failure
 */
item_t scope_func_exit(syntax *const sx, const item_t displ);

#ifdef __cplusplus
} /* extern "C" */
#endif
