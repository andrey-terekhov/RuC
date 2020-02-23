/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define COMPILER_EXPORTED __declspec(dllexport)
#else
#define COMPILER_EXPORTED
#endif

/**
 * Compile RuC files set as compiler arguments
 *
 * @param argc Number of arguments
 * @param argv String arguments to compiler, starting with the name of
 *             compiler executable
 *
 * @return Status code
 */
COMPILER_EXPORTED int compile(int argc, const char *argv[]);

#undef COMPILER_EXPORTED

#ifdef __cplusplus
} /* extern "C" */
#endif
