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

#include "compiler.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


const char *name = "../tests/Fadeev/threads.c";
// "../tests/Egor/Macro/test3.c";
// "../tests/defstest/MULTASSATV_9221.c";
//"../tests/defstest/SHLASSAT_9014.c";
//"../tests/stanfunc0.c";
// "../tests/mips/0test.c";


int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		compiler_compile(name);
	}
	else
	{
		compiler_workspace *ws;

		ws = compiler_get_workspace(argc, argv);
		if (ws == NULL)
		{
			fprintf(stderr, " failed to create a workspace\n");
			return 1;
		}

		if (ws->error.code != COMPILER_WS_EOK)
		{
			char *str;

			str = compiler_workspace_error2str(&ws->error);
			fprintf(stderr, "error: %s", str != NULL ? str : "Unknown workspace error");
			free(str);

			compiler_workspace_free(ws);
			return 1;
		}

		compiler_workspace_compile(ws);
		compiler_workspace_free(ws);
	}

	return 0;
}
