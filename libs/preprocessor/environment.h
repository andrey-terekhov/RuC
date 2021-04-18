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

#include "constants.h"
#include "uniio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum IO_TYPE
{
	file_type = -1,
	macro_text_type,
	param_type,
	if_type,
	while_type,
	define_stack_type,
	all_types,
};

typedef struct environment				/**< Universal io structure */
{
	//local io struct
	char32_t *local_io[all_types];		/**< Pointers to the beginning of all io preprocessor */
	size_t local_io_size[all_types];	/**< Current size of all io preprocessor */

	int curent_io_type;					/**< Current type io preprocessor */
	char32_t *curent_io;				/**< Pointer to the beginning of сurrent io preprocessor */
	size_t curent_io_prt;				/**< Pointer to the current character */

	char32_t macro_tab[MAXTAB];			/**< Io storing macro replacement texts */
	char32_t args[STRING_SIZE * 3];		/**< Io storing parameter texts */
	char32_t macro_stack[STRING_SIZE];	/**< Io storing a pointer to the beginning of the parameter text */
	size_t macro_stack_prt;				/**< Pointer to the current character in io preprocessor*/

	char32_t old_nextchar;				/**< Next symbol received from the file */
	char32_t old_curchar;				/**< Сurrent symbol received from the file */
	int old_io_type[DEPTH_IO_SISE];		/**< Types of nested io */
	char32_t *old_io[DEPTH_IO_SISE];	/**< Pointers of nested io */
	size_t depth_io;					/**< Number of occurrences of io */

	char32_t curchar;					/**< Сurrent symbol */
	char32_t nextchar;					/**< Next symbol */
	//end local io struct

	int hashtab[256];
	int reprtab[MAXTAB];
	int rp;

	char error_string[STRING_SIZE];		/**< Сurrent line scanned from files */
	size_t error_string_size;			/**< Size current line scanned from files */
	const char *curent_path;			/**< The path to the currently processed file */

	
	int cur;
	int nested_if;
	size_t line;

	universal_io *input;				/**< Pointer to the input location */
	universal_io *output;				/**< Pointer to the output location */
	
} environment;

void env_init(environment *const env, universal_io *const output);

/**
 *	Set input of preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	input		Input structure
 *	@param	path		File path
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_set_file_input(environment *const env, universal_io *input, const char* path);

/**
 *	Get current input of preprocessor
 *
 *	@param	evn			Environment structure
 *
 *	@return	@return	Preprocessor input, @c NULL on failure
 */
universal_io *env_get_file_input(environment *const env);

/**
 *	Get output of preprocessor
 *
 *	@param	evn			Environment structure
 *
 *	@return	@return	Preprocessor output, @c NULL on failure
 */
universal_io *env_get_file_output(environment *const env);

/**
 *	Set io preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	buffer		io buffer
 *	@param	type		Type io preprocessor
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_set(environment *const env, char32_t *buffer, int type);

/**
 *	Switch input to new io preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *	@param	prt			Pointer to a character in preprocess io
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_switch_to_new(environment *const env, int type, size_t prt);

/**
 *	Returning input to previous io
 *
 *	@param	evn			Environment structure
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_back_to_previous(environment *const env);

/**
 *	Clear the io preprocessor after the specified character
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *	@param	size		New size io preprocessor
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_clear(environment *const env, int type, size_t size);

/**
 *	Adding symbol to io preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *	@param	simbol		Сharacter to add
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_add_char(environment *const env, int type, char32_t simbol);

/**
 *	Set the symbol in the io preprocessor to the specified location
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *	@param	prt			Pointer to a character in preprocess io
 *	@param	simbol		Сharacter to insert
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_io_set_char(environment *const env, int type, size_t prt, char32_t simbol);

/**
 *	Get symbol to io preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *	@param	prt			Pointer to a character in preprocess io
 *
 *	@return	Desired symbol , @c EOF on failure
 */
char32_t env_io_get_char(environment *const env, int type, size_t prt);

/**
 *	Get current char
 *
 *	@param	evn			Environment structure
 *
 *	@return	Current symbol, @c EOF on failure
 */
char32_t env_get_curchar(environment *const env);

/**
 *	Get next char
 *
 *	@param	evn			Environment structure
 *
 *	@return	Next symbol, @c EOF on failure
 */
char32_t env_get_nextchar(environment *const env);

/**
 *	Scan next character from io
 *
 *	@param	evn			Environment structure
 */
void env_scan_next_char(environment *const env);

/**
 *	Get type of io preprocessor
 *
 *	@param	evn			Environment structure
 *
 *	@return	Type io preprocessor, @c all_types on failure
 */
int env_io_get_type(environment *const env);

/**
 *	Get size of io preprocessor
 *
 *	@param	evn			Environment structure
 *	@param	type		Type io preprocessor
 *
 *	@return	Preprocessor io size, @c SIZE_MAX on failure
 */
size_t env_io_get_size(environment *const env, int type);

/**
 *	Get the number of occurrences of io
 *
 *	@param	evn			Environment structure
 *
 *	@return	Preprocessor io nesting depth, @c SIZE_MAX on failure
 */
size_t env_io_get_depth(environment *const env);

/**
 *	Get a pointer to the current character in io preprocessor
 *
 *	@param	evn			Environment structure
 *
 *	@return	Preprocess current io pointer, @c SIZE_MAX on failure
 */
size_t env_io_get_prt(environment *const env);

/**
 *	Get a pointer to the start of the current macro parameter stack
 *
 *	@param	evn			Environment structure
 *
 *	@return	Define stack pointer, @c SIZE_MAX on failure
 */
size_t env_get_macro_stack_prt(environment *const env);

/**
 *	Move a pointer to the start of the current macro parameter stack
 *
 *	@param	evn			Environment structure
 *	@param	num			Аdded value 
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_move_macro_stack_prt(environment *const env, int num);

/**
 *	Emit an error for some problem
 *
 *	@param	evn			Environment structure
 *	@param	num			Error code
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_error(environment *const env, const int num);

void m_fprintf(environment *const env, int a);

/**
 *	Add a comment to indicate line changes in the output
 *
 *	@param	env	Preprocessor environment
 *
 *	@return	@c 0 on success, @c 1 on failure
 */
int env_add_comment(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
