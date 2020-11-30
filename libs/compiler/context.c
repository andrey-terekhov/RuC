/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov
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

#include "context.h"


compiler_context compiler_context_create(universal_io *const io, syntax *const sx)
{
	compiler_context context;
	context.io = io;
	context.sx = sx;

	context.charnum = 0;
	context.charnum_before = 0;
	context.startmode = 1;
	context.sopnd = -1;
	context.curid = 2;
	context.lg = -1;
	context.displ = -3;
	context.maxdispl = 3;
	context.blockflag = 1;
	context.notrobot = 1;
	context.prdf = -1;
	context.leftansttype = -1;
	context.c_flag = -1;
	context.buf_flag = 0;
	context.error_flag = 0;
	context.new_line_flag = 0;
	context.line = 1;
	context.charnum = 0;
	context.charnum_before = 0;
	context.buf_cur = 0;
	context.temp_tc = 0;

	return context;
}
