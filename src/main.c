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

#ifdef _WIN32
	#pragma comment(linker, "/STACK:268435456")
#endif

#include "compiler.h"
#include "workspace.h"


const char *name = "../tests/executable/output/printf/string_rus.c";
// "../tests/mips/0test.c";


int main(int argc, const char *argv[])
{
	workspace ws = ws_parse_args(argc, argv);

	if (argc < 2)
	{
		ws_add_file(&ws, name);
		ws_add_flag(&ws, "-Wno");
		ws_set_output(&ws, "export.txt");
	}

#ifdef TESTING_EXIT_CODE
	const int ret = compile(&ws) ? TESTING_EXIT_CODE : 0;
#else
	const int ret = compile(&ws);
#endif

	ws_clear(&ws);
	return ret;
}
