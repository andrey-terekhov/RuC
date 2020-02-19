/*
 *  Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "compiler.h"


static const char *name = "../tests/arrays.c";
// "../tests/Egor/Macro/test3.c";
// "../tests/Mishatest.c";
// "../tests/mips/0test.c";


int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		const char *tmp_argv[2] = { argv[0], name };
		compile(2, tmp_argv);
	}
	else
	{
		compile(argc, argv);
	}

	return 0;
}
