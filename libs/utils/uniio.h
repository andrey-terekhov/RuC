/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include "dll.h"
#include "utf8.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct universal_io universal_io;


/**
 *	Prototype of user input/output function
 *
 *	@param	format		String format
 *	@param	args		Variadic argument list
 *
 *	@return	Return printf/scanf-like value
 */
typedef int (*io_user_func)(const char *const format, va_list args);

/**
 *	Prototype of input/output function
 *
 *	@param	io			Universal io structure
 *	@param	format		String format
 *	@param	args		Variadic argument list
 *
 *	@return	Return printf/scanf-like value
 */
typedef int (*io_func)(universal_io *const io, const char *const format, va_list args);


/** Input and output settings */
struct universal_io
{
    FILE *in_file;              /**< Input file */
    const char *in_buffer;      /**< Input buffer */

    size_t in_size;             /**< Size of input buffer */
    size_t in_position;         /**< Current position of input buffer */

    io_user_func in_user_func;  /**< Input user function */
    io_func in_func;            /**< Current input function */

    FILE *out_file;             /**< Output file */
    char *out_buffer;           /**< Output buffer */

    size_t out_size;            /**< Size of output buffer */
    size_t out_position;        /**< Current position of output buffer */

    io_user_func out_user_func; /**< Output user function */
    io_func out_func;           /**< Current output function */
};


/**
 *	Create universal io structure
 *
 *	@return	Universal io structure
 */
EXPORTED universal_io io_create(void);


/**
 *	Set input file
 *
 *	@param	io			Universal io structure
 *	@param	path		Input file path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_set_file(universal_io *const io, const char *const path);

/**
 *	Set input buffer
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Input buffer
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_set_buffer(universal_io *const io, const char *const buffer);

/**
 *	Set input function
 *
 *	@param	io			Universal io structure
 *	@param	func		Input function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_set_func(universal_io *const io, const io_user_func func);

/**
 *	Set input position
 *
 *	@param	io			Universal io structure
 *	@param	position	Input position
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_set_position(universal_io *const io, const size_t position);

/**
 *	Swap input option between two streams
 *
 *	@param	fst			First universal io structure
 *	@param	snd			Second universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_swap(universal_io *const fst, universal_io *const snd);


/**
 *	Check that current input option is correct
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool in_is_correct(const universal_io *const io);

/**
 *	Check that current input option is file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool in_is_file(const universal_io *const io);

/**
 *	Check that current input option is buffer
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool in_is_buffer(const universal_io *const io);

/**
 *	Check that current input option is function
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool in_is_func(const universal_io *const io);


/**
 *	Get input func from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Input function
 */
EXPORTED io_func in_get_func(const universal_io *const io);

/**
 *	Get input file path from universal io structure
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Buffer to return file path
 *
 *	@return	Size of buffer
 */
EXPORTED size_t in_get_path(const universal_io *const io, char *const buffer);

/**
 *	Get input buffer from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Input buffer
 */
EXPORTED const char *in_get_buffer(const universal_io *const io);

/**
 *	Get input position from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Input position
 */
EXPORTED size_t in_get_position(const universal_io *const io);


/**
 *	Close input file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_close_file(universal_io *const io);

/**
 *	Clear all input parameters
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int in_clear(universal_io *const io);


/**
 *	Set output file
 *
 *	@param	io			Universal io structure
 *	@param	path		Output file path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_set_file(universal_io *const io, const char *const path);

/**
 *	Set output buffer
 *
 *	@param	io			Universal io structure
 *	@param	size		Output buffer size
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_set_buffer(universal_io *const io, const size_t size);

/**
 *	Set output function
 *
 *	@param	io			Universal io structure
 *	@param	func		Output function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_set_func(universal_io *const io, const io_user_func func);

/**
 *	Swap output option between two streams
 *
 *	@param	fst			First universal io structure
 *	@param	snd			Second universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_swap(universal_io *const fst, universal_io *const snd);


/**
 *	Check that current output option is correct
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool out_is_correct(const universal_io *const io);

/**
 *	Check that current output option is file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool out_is_file(const universal_io *const io);

/**
 *	Check that current output option is buffer
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool out_is_buffer(const universal_io *const io);

/**
 *	Check that current output option is function
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool out_is_func(const universal_io *const io);


/**
 *	Get output function from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Output function
 */
EXPORTED io_func out_get_func(const universal_io *const io);

/**
 *	Get output file path from universal io structure
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Buffer to return file path
 *
 *	@return	Size of buffer
 */
EXPORTED size_t out_get_path(const universal_io *const io, char *const buffer);


/**
 *	Extract output buffer from universal io structure
 *
 *	@param	io		Command line arguments
 *
 *	@return	Output buffer (need to use @c free() function)
 */
EXPORTED char *out_extract_buffer(universal_io *const io);

/**
 *	Close output file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_close_file(universal_io *const io);

/**
 *	Clear all output parameters
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int out_clear(universal_io *const io);


/**
 *	Clear universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int io_erase(universal_io *const io);

#ifdef __cplusplus
} /* extern "C" */
#endif
