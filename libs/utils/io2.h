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

#include <stdio.h>
#include <stddef.h>
#include "utils_internal.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Prototype of user function
 *
 *	@param	format		
 *	@param	args		List of arguments
 */
typedef int (*io_user_func)(const char *const format, va_list args);

/**
 *	Prototype of function
 *
 *	@param	format		
 *	@param	args		List of arguments
 */
typedef int (*io_func)(const char *const format, va_list args);


/** Structure for parsing start arguments of program */
typedef struct universal_io
{
	FILE *in_file;				/** Input file path */
	const char *in_buffer;		/** Input buffer */

	size_t in_size;				/**  */
	size_t in_position;			/**  */

	io_user_func in_user_func;	/**  */
	io_func in_func;			/**  */

	FILE *out_file;				/**  */
	char *out_buffer;			/**  */

	size_t out_size;			/**  */
	size_t out_position;		/**  */

	io_user_func out_user_func;	/**  */
	io_func out_func;			/**  */
} universal_io;


/**
 *	Create universal io structure
 *
 *	@return	Universal io structure
 */
UTILS_EXPORTED universal_io io_create();


/**
 *	Set input file
 *
 *	@param	io			Universal io structure
 *	@param	path		Input file path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int in_set_file(universal_io *const io, const char *const path);

/**
 *	Set input buffer
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Input buffer
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int in_set_buffer(universal_io *const io, const char *const buffer);

/**
 *	Set input function
 *
 *	@param	io			Universal io structure
 *	@param	func		Input function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int in_set_func(universal_io *const io, const io_user_func func);


/**
 *	Check that in_file is file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int in_is_file(const universal_io *const io);

/**
 *	Check that in_buffer is buffer
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int in_is_buffer(const universal_io *const io);

/**
 *	Check that in_func is function
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int in_is_func(const universal_io *const io);


/**
 *	Get input func from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Input function
 */
UTILS_EXPORTED io_func in_get_func(const universal_io *const io);

/**
 *	Get input file path from universal io structure
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Buffer to return
 *
 *	@return	Size of buffer
 */
UTILS_EXPORTED size_t in_get_path(const universal_io *const io, char *const buffer);

/**
 *	Get input buffer from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Input buffer
 */
UTILS_EXPORTED const char *in_get_buffer(const universal_io *const io);

/**
 *	Get input position from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	input position
 */
UTILS_EXPORTED size_t in_get_position(const universal_io *const io);


/**
 *	Close input file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int in_close_file(universal_io *const io);

/**
 *	Clear input
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int in_clear(universal_io *const io);


/**
 *	Set output file
 *
 *	@param	io			Universal io structure
 *	@param	path		Output file path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int out_set_file(universal_io *const io, const char *const path);

/**
 *	Set output buffer
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Output buffer
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int out_set_buffer(universal_io *const io, const char *const buffer);	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/**
 *	Set output function
 *
 *	@param	io			Universal io structure
 *	@param	func		Output function
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int out_set_func(universal_io *const io, const io_user_func func);


/**
 *	Chek that out_file is file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int out_is_file(const universal_io *const io);

/**
 *	Chek that out_buffer is buffer
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int out_is_buffer(const universal_io *const io);

/**
 *	Chek that out_func is function
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
UTILS_EXPORTED int out_is_func(const universal_io *const io);


/**
 *	Get output function from universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	Output function
 */
UTILS_EXPORTED io_func out_get_func(const universal_io *const io);

/**
 *	Get output file path from universal io structure
 *
 *	@param	io			Universal io structure
 *	@param	buffer		Buffer to return file path
 *
 *	@return	Size of buffer
 */
UTILS_EXPORTED size_t out_get_path(const universal_io *const io, char *const buffer);


/**
 *	Extract output buffer from universal io structure
 *
 *	@param	io		Command line arguments
 *
 *	@return	Output buffer
 */
UTILS_EXPORTED char *out_extract_buffer(universal_io *const io);

/**
 *	Close output file
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int out_close_file(universal_io *const io);

/**
 *	Clear output 
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int out_clear(universal_io *const io);


/**
 *	Clear universal io structure
 *
 *	@param	io			Universal io structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
UTILS_EXPORTED int io_erase(universal_io *const io);

#ifdef __cplusplus
} /* extern "C" */
#endif
