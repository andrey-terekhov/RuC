/*
 *	Copyright 2022 Andrey Terekhov, Maxim Menshikov
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

/** Analysis result */
typedef enum ANALYSIS_RESULT
{
	ANALYSIS_RESULT_OK,				/**< No errors found */
	ANALYSIS_RESULT_WARNINGS_FOUND,	/**< Warnings emitted */
	ANALYSIS_RESULT_ERRORS_FOUND,	/**< Errors emitted */
    ANALYSIS_RESULT_INTERNAL_ERROR, /**< Internal error occurred */
} analysis_result_t;

EXPORTED analysis_result_t analyze(workspace *const ws);

#ifdef __cplusplus
} /* extern "C" */
#endif
