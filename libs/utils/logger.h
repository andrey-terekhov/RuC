/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov 
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

typedef void (*log)(const char *const tag, const char *const msg);


int set_error_log(const log func);
int set_warning_log(const log func);
int set_note_log(const log func);

void log_error(const char *const tag, const char *const msg, const char *const line, const size_t symbol);
void log_warning(const char *const tag, const char *const msg, const char *const line, const size_t symbol);
void log_note(const char *const tag, const char *const msg, const char *const line, const size_t symbol);

void log_system_error(const char *const tag, const char *const msg);
void log_system_warning(const char *const tag, const char *const msg);
void log_system_note(const char *const tag, const char *const msg);

#ifdef __cplusplus
} /* extern "C" */
#endif
