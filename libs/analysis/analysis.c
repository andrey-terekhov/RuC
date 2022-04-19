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
#include <stdlib.h>
#include <stdio.h>
#include "workspace.h"
#include "analysis.h"
#include <asp/asp_simple.h>

static void _asp_report_cb(asp_report *report, void *opaque)
{
	(void)report;
	(void)opaque;
}

analysis_result_t analyze(workspace *const ws)
{
	asp_status rc;
	asp_simple_connection     conn;
	asp_simple_params         params;
	asp_simple_diagnostics    diag;
	const char              **paths;
	size_t                    file_count = ws_get_files_num(ws);
	size_t                    i;

	paths = calloc(file_count + 1, sizeof(char *));

	for (i = 0; i < file_count; ++i)
	{
		paths[i] = ws_get_file(ws, i);
		printf("PATH: %s\n", paths[i]);
	}

	asp_simple_connection_init(&conn, NULL);
	asp_simple_params_init(&params);
	asp_simple_diagnostics_init(&diag, _asp_report_cb);

	params.filenames = paths;
	params.language = ASP_LANGUAGE_RUC;
	rc = asp_simple_invoke(&conn, &params, &diag);
	free(params.filenames);
	if (rc != ASP_EOK)
		return ANALYSIS_RESULT_INTERNAL_ERROR;
	return ANALYSIS_RESULT_OK;
}
