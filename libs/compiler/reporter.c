/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "reporter.h"
#include <string.h>


/**
 *	Check if error recovery disabled
 *
 *	@param	ws			Compiler workspace
 *
 *	@return	Recovery status
 */
static inline bool recovery_status(const workspace *const ws)
{
	for (size_t i = 0; ; i++)
	{
		const char *flag = ws_get_flag(ws, i);
		if (flag == NULL)
		{
			return false;
		}
		else if (strcmp(flag, "-Wno") == 0)
		{
			return true;
		}
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


reporter reporter_create(const workspace *const ws)
{
	reporter rprt;
	rprt.is_recovery_disabled = recovery_status(ws);
	rprt.errors = 0;
	rprt.warnings = 0;

	return rprt;
}

size_t reporter_get_errors_number(reporter *const rprt)
{
	return rprt->errors;
}

void report_error(reporter *const rprt, universal_io *const io, const location loc, const err_t num, va_list args)
{
	if (rprt->is_recovery_disabled && rprt->errors != 0)
	{
		return;
	}

	const size_t prev_loc = in_get_position(io);
	in_set_position(io, loc.begin);

	verror(io, num, args);
	rprt->errors++;

	in_set_position(io, prev_loc);
}

void report_warning(reporter *const rprt, universal_io *const io, const location loc, const warning_t num, va_list args)
{
	if (rprt->is_recovery_disabled && rprt->errors != 0)
	{
		return;
	}

	const size_t prev_loc = in_get_position(io);
	in_set_position(io, loc.begin);

	vwarning(io, num, args);
	rprt->warnings++;

	in_set_position(io, prev_loc);
}
