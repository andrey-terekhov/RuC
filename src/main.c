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

#ifdef _MSC_VER
	#pragma comment(linker, "/STACK:268435456")
#endif

#include "compiler.h"
#include "workspace.h"
#include "hash.h"


const char *name = "../tests/executable/structures/SELECT_9459.c";
// "../tests/mips/0test.c";


int main(int argc, const char *argv[])
{
	hash hs = hash_create(0);

	size_t index = hash_add(&hs, 1, 1);
	printf("адын %zu\n", index);
	

	size_t index2 = hash_add(&hs, 257, 1);
	printf("257 %zu\n", index2);

	hash_remove(&hs, 1);

	index = hash_add(&hs, 1, 1);
	printf("удалил 1 и записал 1 %zu\n", index);

	index = hash_set(&hs, 257, 0, 2);
	printf("set 257 %zu\n", index);

	index2 = hash_set_by_index(&hs, index2, 0, 2);
	printf("set_by_index 257 %zu\n", index2);

	index2 = hash_get_index(&hs, 1);
	printf("getindex %zu\n", index2);







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
