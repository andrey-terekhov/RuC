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

/* compiler headers */
#include "AST.h"
#include "builder.h"
#include "codegen.h"
#include "compiler.h"
#include "errors.h"
#include "instructions.h"
#include "lexer.h"
#include "llvmgen.h"
#include "operations.h"
#include "parser.h"
#include "reporter.h"
#include "syntax.h"
#include "token.h"
#include "writer.h"

/* preprocessor headers */
#ifdef RUC_CPPWRAP_CHECK_PREPROCESSOR
	#include "calculator.h"
	#include "constants.h"
	#include "environment.h"
	#include "error.h"
	#include "linker.h"
	#include "macro_load.h"
	#include "macro_save.h"
	#include "parser.h"
	#include "preprocessor.h"
	#include "utils.h"
#endif

/* utils headers */
#include "commenter.h"
#include "dll.h"
#include "hash.h"
#include "item.h"
#include "logger.h"
#include "map.h"
#include "node_vector.h"
#include "stack.h"
#include "strings.h"
#include "tree.h"
#include "uniio.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include "utf8.h"
#include "vector.h"
#include "workspace.h"
