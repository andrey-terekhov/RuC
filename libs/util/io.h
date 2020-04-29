/*
 *	Copyright 2019 Andrey Terekhov
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

/**
 * Input/Output pipe type
 */
typedef enum ruc_io_type
{
	IO_TYPE_INPUT,	/** Input pipe */
	IO_TYPE_OUTPUT, /** Output pipe */
	IO_TYPE_ERROR,	/** Error pipe */
	IO_TYPE_MISC,	/** Misc output pipe */
} ruc_io_type;

typedef enum ruc_io_source
{
	IO_SOURCE_FILE, /** File-based input/output */
	IO_SOURCE_MEM,	/** Buffer-based input/output */
} ruc_io_source;
