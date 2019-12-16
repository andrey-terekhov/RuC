//
//  global_vars.h
//  RuC
//
//  Created by Andrey Terekhov on 03/06/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
/// Users/ant/Desktop/RuC/RuC/main.c

#include <stdio.h>

#ifndef RuC_global_vars_h
#define RuC_global_vars_h

#include "Defs.h"
#include "context.h"

#define UNUSED(x) (void)(x)

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#endif
