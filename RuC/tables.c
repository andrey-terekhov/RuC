#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global_vars.h"

int
toreprtab(ruc_context *context, char str[])
{
    int i, oldrepr = context->rp;
    context->hash = 0;
    context->rp += 2;
    for (i = 0; str[i] != 0; i++)
    {
        context->hash += str[i];
        context->reprtab[context->rp++] = str[i];
    }
    context->hash &= 255;
    context->reprtab[context->rp++] = 0;
    context->reprtab[oldrepr] = context->hashtab[context->hash];
    context->reprtab[oldrepr + 1] = 1;
    return context->hashtab[context->hash] = oldrepr;
}

/* Инициализация modetab */
void
init_modetab(ruc_context *context)
{
    // занесение в modetab описателя struct {int numTh; int inf; }
    context->modetab[1] = 0;
    context->modetab[2] = MSTRUCT;
    context->modetab[3] = 2;
    context->modetab[4] = 4;
    context->modetab[5] = context->modetab[7] = LINT;
    context->modetab[6] = toreprtab(context, "numTh");
    context->modetab[8] = toreprtab(context, "data");

    // занесение в modetab описателя функции void t_msg_send(struct msg_info m)
    context->modetab[9] = 1;
    context->modetab[10] = MFUNCTION;
    context->modetab[11] = LVOID;
    context->modetab[12] = 1;
    context->modetab[13] = 2;

    // занесение в modetab описателя функции void* interpreter(void* n)
    context->modetab[14] = 9;
    context->modetab[15] = MFUNCTION;
    context->modetab[16] = LVOIDASTER;
    context->modetab[17] = 1;
    context->modetab[18] = LVOIDASTER;
    context->modetab[19] = context->startmode = 14;
    context->md = 19;
    context->keywordsnum = 0;
    context->lines[context->line = 1] = 1;
    context->charnum = 1;
    context->kw = 1;
    context->tc = 0;
}
