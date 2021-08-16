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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "errors.h"
#include "map.h"
#include "strings.h"
#include "tree.h"
#include "vector.h"


#define MAX_STRING_LENGTH 128


#ifdef __cplusplus
extern "C" {
#endif

/** Type qualifiers */
enum TYPE
{
	TYPE_FILE			= -7,
	TYPE_VOID,
	TYPE_FLOATING		= -3,
	TYPE_INTEGER		= -1,
	TYPE_UNDEFINED,

	TYPE_MSG_INFO 		= 2,
	TYPE_VOID_POINTER	= 10,
	TYPE_FUNCTION		= 1001,
	TYPE_STRUCTURE,
	TYPE_ARRAY,
	TYPE_POINTER,
};

/** Value category */
typedef enum VALUE
{
	LVALUE,		/**< An expression that designates an object */
	RVALUE,		/**< An expression detached from any specific storage */
} category_t;


typedef struct location
{
	size_t begin;
	size_t end;
} location;

/** Global vars definition */
typedef struct syntax
{
	universal_io *io;			/**< Universal io structure */
	node nd;					/**< Node for expression subtree [temp] */

	strings string_literals;	/**< String literals list */

	vector predef;				/**< Predefined functions table */
	vector functions;			/**< Functions table */

	vector tree;				/**< Tree table */

	vector identifiers;			/**< Identifiers table */
	size_t cur_id;				/**< Start of current scope in identifiers table */

	vector types;				/**< Types table */
	size_t start_type;			/**< Start of last record in types table */

	map representations;		/**< Representations table */

	item_t max_displ;			/**< Max displacement */
	item_t max_displg;			/**< Max displacement */

	item_t displ;				/**< Stack displacement in current scope */
	item_t lg;					/**< Displacement from l (+1) or g (-1) */

	size_t procd;				/**< Process management daemon */
	size_t ref_main;			/**< Main function reference */
	
	bool was_error;				/**< Set, if was error */
} syntax;


/**
 *	Create Syntax structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Syntax structure
 */
syntax sx_create(universal_io *const io);

/**
 *	Check if syntax structure is correct
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool sx_is_correct(syntax *const sx);

/**
 *	Free allocated memory
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int sx_clear(syntax *const sx);


/**
 *	Add new dynamic UTF-8 string to string literal vector
 *
 *	@param	sx				Syntax structure
 *	@param	str				Dynamic UTF-8 string
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
size_t string_add(syntax *const sx, const vector *const str);

/**
 *	Get string
 *
 *	@param	sx				Syntax structure
 *	@param	index			Index
 *
 *	@return	String, @c NULL on failure
 */
const char* string_get(const syntax *const sx, const size_t index);


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
 *	@param	kind		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	type		New identifier type
 *	@param	func_def	@c 0 for function without args,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for variables
 *
 *	@return	Index of the last item in identifiers table,
 *			@c SIZE_MAX @c - @c 1 on redeclaration
 *			@c SIZE_MAX on redefinition of main
 */
size_t ident_add(syntax *const sx, const size_t repr, const item_t kind, const item_t type, const int func_def);

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
 *	Get item type from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier type, @c ITEM_MAX on failure
 */
item_t ident_get_type(const syntax *const sx, const size_t index);

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
 *	Set identifier type by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	type		Identifier type
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_type(syntax *const sx, const size_t index, const item_t type);

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
 *	Add a new record to types table
 *
 *	@param	sx			Syntax structure
 *	@param	record		Pointer to the new record
 *	@param	size		Size of the new record
 *
 *	@return	New type, @c ITEM_MAX on failure
 */
item_t type_add(syntax *const sx, const item_t *const record, const size_t size);

/**
 *	Get an item from types table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in types table
 *
 *	@return	Item by index from types table, @c ITEM_MAX on failure
 */
item_t type_get(const syntax *const sx, const size_t index);

/**
 *	Get type size
 *
 *	@param	sx			Syntax structure
 *	@param	type		Standard type or index of the types table
 *
 *	@return	Type size
 */
size_t type_size(const syntax *const sx, const item_t type);

/**
 *	Check if type is integer
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_integer(const item_t type);

/**
 *	Check if type is floating
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_floating(const item_t type);

/**
 *	Check if type is arithmetic
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_arithmetic(const item_t type);

/**
 *	Check if type is void
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_void(const item_t type);

/**
 *	Check if type is array
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_array(const syntax *const sx, const item_t type);

/**
 *	Check if type is structure
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_structure(const syntax *const sx, const item_t type);

/**
 *	Check if type is function
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_function(const syntax *const sx, const item_t type);

/**
 *	Check if type is pointer
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_pointer(const syntax *const sx, const item_t type);

/**
 *	Check if type is scalar
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_scalar(const syntax *const sx, const item_t type);

/**
 *	Check if type is aggregate
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_aggregate(const syntax *const sx, const item_t type);

/**
 *	Check if type is string
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_string(const syntax *const sx, const item_t type);

/**
 *	Check if type is struct pointer
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_struct_pointer(const syntax *const sx, const item_t type);

/**
 *	Check if type is undefined
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_undefined(const item_t type);

/**
 *	Check if type is FILE
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_file(const item_t type);

/**
 *	Get element type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Array type
 *
 *	@return	Element type, @c ITEM_MAX on failure
 */
item_t type_array_get_element_type(const syntax *const sx, const item_t type);

/**
 *	Check if structure has a name
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_structure_has_name(const syntax *const sx, const item_t type);

/**
 *	Get structure name
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *
 *	@return	Structure name, @c SIZE_MAX on failure
 */
size_t type_structure_get_name(const syntax *const sx, const item_t type);

/**
 *	Get member amount
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *
 *	@return	Member amount, @c SIZE_MAX on failure
 */
size_t type_structure_get_member_amount(const syntax *const sx, const item_t type);

/**
 *	Get member name by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *	@param	index		Member number
 *
 *	@return	Member name, @c SIZE_MAX on failure
 */
size_t type_structure_get_member_name(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get member type by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *	@param	index		Member number
 *
 *	@return	Member type, @c ITEM_MAX on failure
 */
item_t type_structure_get_member_type(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get return type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *
 *	@return	Return type, @c ITEM_MAX on failure
 */
item_t type_function_get_return_type(const syntax *const sx, const item_t type);

/**
 *	Get parameter amount
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *
 *	@return	Parameter amount, @c SIZE_MAX on failure
 */
size_t type_function_get_parameter_amount(const syntax *const sx, const item_t type);

/**
 *	Get parameter type by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *	@param	index		Parameter number
 *
 *	@return	Parameter type, @c ITEM_MAX on failure
 */
item_t type_function_get_parameter_type(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get element type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Pointer type
 *
 *	@return	Element type, @c ITEM_MAX on failure
 */
item_t type_pointer_get_element_type(const syntax *const sx, const item_t type);

/**
 *	Create array type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Element type
 *
 *	@return	Array type
 */
item_t type_array(syntax *const sx, const item_t type);

/**
 *	Create function type
 *
 *	@param	sx			Syntax structure
 *	@param	return_type	Return type
 *	@param	args		List of argument types
 *
 *	@return	Function type
 */
item_t type_function(syntax *const sx, const item_t return_type, const char *const args);

/**
 *	Create pointer type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Referenced type
 *
 *	@return	Pointer type
 */
item_t type_pointer(syntax *const sx, const item_t type);


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


/**
 *	Get amount of strings
 *
 *	@param	sx	Syntax structure
 *
 *	@return	Amount of strings
 */
size_t strings_amount(const syntax *const sx);


/**
 *	Get expression type
 *
 *	@param	nd	Expression
 *
 *	@return	Expression type
 */
inline item_t expression_get_type(const node *const nd)
{
	return node_get_arg(nd, 0);
}

/**
 *	Check if expression is lvalue
 *
 *	@param	nd	Expression for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool expression_is_lvalue(const node *const nd)
{
	return node_get_arg(nd, 1) == LVALUE;
}

/**
 *	Get expression location
 *
 *	@param	nd	Expression
 *
 *	@return	Expression location
 */
inline location expression_get_location(const node *const nd)
{
	const size_t argc = node_get_argc(nd);
	return (location){ (size_t)node_get_arg(nd, argc - 2), (size_t)node_get_arg(nd, argc - 1) };
}


#ifdef __cplusplus
} /* extern "C" */
#endif
