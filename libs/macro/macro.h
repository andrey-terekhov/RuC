/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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
#include "workspace.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Preprocess files from workspace
 *
 *	@param	ws		Workspace
 *
 *	@return	Preprocessed string, @c NULL on failure
 */
EXPORTED char *macro(workspace *const ws);

/**
 *	Preprocess files from workspace
 *
 *	@param	ws		Workspace
 *	@param	path	Output file name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int macro_to_file(workspace *const ws, const char *const path);


/**
 *	Preprocess files from terminal arguments
 *
 *	@param	argc	Number of command line arguments
 *	@param	argv	Command line arguments
 *
 *	@return	Preprocessed string, @c NULL on failure
 */
EXPORTED char *auto_macro(const int argc, const char *const *const argv);

/**
 *	Preprocess files from terminal arguments
 *
 *	@param	argc	Number of command line arguments
 *	@param	argv	Command line arguments
 *	@param	path	Output file name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path);

#ifdef __cplusplus
} /* extern "C" */
#endif
