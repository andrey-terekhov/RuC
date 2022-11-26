/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev
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

#include "stack.h"


extern stack stack_create(const size_t alloc);

extern int stack_push(stack *const stk, const item_t value);
extern int stack_push_double(stack *const stk, const double value);
extern int stack_push_int64(stack *const stk, const int64_t value);

extern item_t stack_pop(stack *const stk);
extern double stack_pop_double(stack *const stk);
extern int64_t stack_pop_int64(stack *const stk);

extern item_t stack_peek(const stack *const stk);
extern double stack_peek_double(const stack *const stk);
extern int64_t stack_peek_int64(const stack *const stk);

extern int stack_reset(stack *const stk);
extern size_t stack_size(const stack *const stk);
extern bool stack_is_correct(const stack *const stk);

extern int stack_clear(stack *const stk);
