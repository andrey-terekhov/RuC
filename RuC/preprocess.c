#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global_vars.h"
#include "scanner.h"
#include "error.h"

/* Forward declarations */
static int mletter(ruc_context *context, int r);
static int mdigit(ruc_context *context, int r);
static int mequal(ruc_context *context, int str[], int j);

static void mend_line(ruc_context *context);
static void m_nextch(ruc_context *context, int i);
static void m_fprintf(ruc_context *context, int a);

static void to_macrotext(ruc_context *, char chang[], int oldrepr); //
static void macro_reprtab(ruc_context *, char chang[]);
static void from_macrotext(ruc_context *); // 5
static int  macro_keywords(ruc_context *); // 12
static void relis_define(ruc_context *); // 2

static void toreprtab_f(ruc_context *);
static void to_functionident(ruc_context *); // 4
static int  scob(ruc_context *, int cp); // 6
static void from_functionident(ruc_context *, int r);
static void create_change(ruc_context *, int r1); // 11
static void r_macrofunction(ruc_context *); // 3

// void m_ident();//5
static int find_ident(ruc_context *);

static int  check_if(ruc_context *, int type_if); // 10
static void end_line(ruc_context *); // 9
static void false_if(ruc_context *); // 8
static int  m_false(ruc_context *); // 7
static void m_true(ruc_context *, int type_if);
static void m_if(ruc_context *, int type_if);

static void macroscan(ruc_context *context); // 1,17
void preprocess_file(ruc_context *context); // 18

void
show_macro(ruc_context *context)
{
    int i1 = context->lines[context->line];
    int str1[50];
    int j = 0;
    int k;
    int flag = 1;
    context->arg = context->mlines[context->m_conect_lines[context->line]];

    context->flag_show_macro = 1;
    while (i1 < context->charnum)
    {
        // printf("\nbe[context->arg= %i] = %i, so[i1] = %i",context->arg,
        // context->before_source[context->arg],context->source[i1] );
        if (context->source[i1] == context->before_source[context->arg])
        {
            str1[j++] = context->before_source[context->arg];
            i1++;
            context->arg++;
        }
        else
        {
            flag = 0;
            context->curchar = context->before_source[context->arg];
            from_macrotext(context);

            i1 += context->msp;
        }
    }

    printer_printf(&context->miscout_options, "line %i) ",
                   context->m_conect_lines[context->line]);

    for (k = 0; k < j; k++)
    {
        printer_printchar(&context->miscout_options, str1[k]);
    }
    if (flag == 0)
    {
        printer_printf(&context->miscout_options,
                       "\n В строке есть макрозамена, строка после "
                       "макрогенерации:\nline %i)",
                       context->m_conect_lines[context->line]);
        for (k = context->lines[context->line - 1]; k < context->charnum; k++)
        {
            printer_printchar(&context->miscout_options, context->source[k]);
        }
    }
}

static int
mletter(ruc_context *context, int r)
{
    UNUSED(context);
    return (r >= 'A' && r <= 'Z') || (r >= 'a' && r <= 'z') || r == '_' ||
        (r >= 0x410 /*А */ && r <= 0x44F /*'я'*/);
}

static int
mdigit(ruc_context *context, int r)
{
    UNUSED(context);
    return r >= '0' && r <= '9';
}

static int
mequal(ruc_context *context, int str[], int j)
{
    int i = 0;
    while (str[i++] == context->functionident[j++])
    {
        if (str[i] == 0 && context->functionident[j] == 0)
            return 1;
    }
    return 0;
}

static void
mend_line(ruc_context *context)
{
    int j;
    if (context->flag_show_macro == 0)
    {
        context->mlines[++context->mline] = context->m_charnum;
        context->mlines[context->mline + 1] = context->m_charnum;
        if (context->kw)
        {
            printer_printf(&context->miscout_options, "Line %i) ",
                           context->mline - 1);
            for (j = context->mlines[context->mline - 1];
                 j < context->mlines[context->mline]; j++)
                if (context->before_source[j] != EOF)
                    printer_printchar(&context->miscout_options,
                                      context->before_source[j]);
        }
    }

    return;
}

static void
monemore(ruc_context *context)
{
    if (context->flag_show_macro == 0)
    {
        context->curchar = context->nextchar;
        context->nextchar = getnext(context);
        context->before_source[context->m_charnum++] = context->curchar;
    }
    else
    {
        context->curchar = context->before_source[context->arg++];
    }

    if (context->curchar == EOF)
    {
        mend_line(context);
        printer_printf(&context->miscout_options, "\n");
        return;
    }
}

static void
m_nextch(ruc_context *context, int i)
{
    // printf(" i = %d curcar = %c curcar = %i\n", i, context->curchar,
    // context->curchar);
    monemore(context);

    if (context->curchar == '/' && context->nextchar == '/')
    {
        if (i > 13)
        {
            m_fprintf(context, context->curchar);
        }
        do
        {
            monemore(context);
            if (i > 13)
            {
                m_fprintf(context, context->curchar);
            }
        } while (context->curchar != '\n');

        mend_line(context);
        return;
    }

    if (context->curchar == '/' && context->nextchar == '*')
    {
        if (i > 13)
        {
            m_fprintf(context, context->curchar);
        }

        monemore(context);
        if (i > 13)
        {
            m_fprintf(context, context->curchar);
        }
        do
        {
            monemore(context);
            if (i > 13)
            {
                m_fprintf(context, context->curchar);
            }

            if (context->curchar == EOF)
            {
                mend_line(context);
                printf("\n");
                m_error(context, comm_not_ended);
            }
            if (context->curchar == '\n')
                mend_line(context);
        } while (context->curchar != '*' || context->nextchar != '/');

        monemore(context);
        if (i > 13)
        {
            m_fprintf(context, context->curchar);
        }
        context->curchar = ' ';
        return;
    }

    if (context->curchar == '\n')
    {
        mend_line(context);
    }
    return;
}

static void
m_fprintf(ruc_context *context, int a)
{
    if (a == '\n')
    {
        context->m_conect_lines[context->mcl++] = context->mline - 1;
    }
    printer_printchar(&context->output_options, a);
    //_obsolete_fprintf_char(context->output_options.output, a);

    return;
}

static void
to_macrotext(ruc_context *context, char chang[], int oldrepr)
{
    int i;
    context->macrotext[context->mp++] = oldrepr;
    for (i = 0; chang[i] != 0; i++)
    {
        context->macrotext[context->mp++] = chang[i];
    }
    context->macrotext[context->mp++] = 0;
}

static void
macro_reprtab(ruc_context *context, char chang[])
{
    int oldrepr = context->rp;
    int r, i;


    context->mlastrp = oldrepr;
    context->hash = 0;
    context->rp += 2;

    for (i = 0; i < context->msp; i++)
    {
        context->hash += context->mstring[i];
        context->reprtab[context->rp++] = context->mstring[i];
    }
    context->msp = 0;
    context->hash &= 255;
    context->reprtab[context->rp++] = 0;
    context->reprtab[oldrepr] = context->hashtab[context->hash];
    context->reprtab[oldrepr + 1] = context->mp;

    r = context->hashtab[context->hash];
    while (r != 0)
    {
        r = context->reprtab[r];
    }
    to_macrotext(context, chang, oldrepr);
    context->hashtab[context->hash] = oldrepr;
}

static void
from_macrotext(ruc_context *context)
{
    int r;
    context->msp = 0;

    while (letter(context) || digit(context))
    {
        context->mstring[context->msp++] = context->curchar;
        m_nextch(context, 5);
    }

    r = find_ident(context);
    // printf("r = %d\n", r);

    if (r)
    {
        context->msp = 0;
        if (context->reprtab[r + 1] == 2)
        {
            from_functionident(context, r);
            return;
        }

        r = context->reprtab[r + 1] + 1;

        for (; context->macrotext[r] != 0; r++)
        {
            context->mstring[context->msp++] = context->macrotext[r];
        }
    }

    return;
}

static int
macro_keywords(ruc_context *context)
{
    int oldrepr = context->rp;
    int r = 0;

    context->rp += 2;
    context->hash = 0;

    do
    {
        context->hash += context->curchar;
        context->reprtab[context->rp++] = context->curchar;
        m_nextch(context, 12);
    } while (letter(context) || digit(context));

    if (context->curchar != ' ' && context->curchar != '\n' &&
        context->curchar != '\t')
    {
        m_error(context, after_ident_must_be_space);
    }
    else
    {
        m_nextch(context, 12);
    }

    context->hash &= 255;
    context->reprtab[context->rp++] = 0;
    r = context->hashtab[context->hash];
    if (r)
    {
        do
        {
            if (equal(context, r, oldrepr))
            {
                context->rp = oldrepr;
                return (context->reprtab[r + 1] < 0)
                    ? context->reprtab[r + 1]
                    : (context->repr = r, IDENT);
            }
            else
                r = context->reprtab[r];
        } while (r);
    }
    return 0;
}

static void
relis_define(ruc_context *context)
{

    if (letter(context))
    {
        context->msp = 0;
        while (letter(context) || digit(context))
        {
            context->mstring[context->msp++] = context->curchar;
            m_nextch(context, 2);
        }

        if (find_ident(context) != 0)
        {
            m_error(context, repeat_ident);
        }

        if (context->curchar == '(')
        {
            // printf("str = %s\n", mstring);
            toreprtab_f(context);
            m_nextch(context, 2);
            r_macrofunction(context);
            return;
        }
        else if (context->curchar != ' ')
        {
            m_error(context, after_ident_must_be_space);
        }
        else
        {
            int i = 0;
            m_nextch(context, 2);
            char chang[30] = { "\0" };
            while (context->curchar != '\n')
            {
                chang[i++] = context->curchar;
                m_nextch(context, 2);
                if (context->curchar == EOF)
                {
                    m_error(context, not_end_fail_preprocess);
                }
            }
            macro_reprtab(context, chang);
            return;
        }
    }
    else
    {
        m_error(context, ident_begins_with_letters);
    }
}

static void
toreprtab_f(ruc_context *context)
{
    int i;
    int oldrepr = context->rp;
    context->mlastrp = oldrepr;
    // printf("r = %i\n", oldrepr);
    context->hash = 0;
    context->rp += 2;
    for (i = 0; i < context->msp; i++)
    {
        context->hash += context->mstring[i];
        context->reprtab[context->rp++] = context->mstring[i];
    }

    context->hash &= 255;
    context->reprtab[context->rp++] = 0;
    context->reprtab[context->rp++] = context->fip;
    context->reprtab[context->rp++] = 0;
    context->reprtab[oldrepr] = context->hashtab[context->hash];
    context->reprtab[oldrepr + 1] = 2;
    context->hashtab[context->hash] = oldrepr;
}

static void
to_functionident(ruc_context *context)
{
    while (context->curchar != ')')
    {
        // reportab
        context->msp = 0; //   \/
        context->fip++; // funcid 5[] -> конец = 13
        if (letter(context)) //       6[] -> macrofunc
        { //       7[] -> fcang
          //       8[a]
            while (letter(context) || digit(context)) //       9[0]
            { //      10[] -> fcang
                context->functionident[context->fip++] =
                    context->curchar; //      11[b]
                context->mstring[context->msp++] =
                    context->curchar; //      12[0]
                m_nextch(context, 4);
            }
            if (find_ident(context) != 0)
            {
                m_error(context, repeat_ident);
            }
            context->functionident[context->fip++] = 0;
        }
        else
        {
            m_error(context, functionid_begins_with_letters);
        }
        context->msp = 0;
        if (context->curchar == ',' && context->nextchar == ' ')
        {
            m_nextch(context, 4);
            m_nextch(context, 4);
        }
        else if (context->curchar != ')')
        {
            m_error(context, after_functionid_must_be_comma);
        }
    }
    m_nextch(context, 4);
    return;
}

static void
from_functionident(ruc_context *context, int r)
{
    int i, kp, cp;
    int r1 = r + 2;
    int str[30];

    for (; context->reprtab[r1] != 0; r1++)
        ;
    r1++;
    r1 = context->reprtab[r1];
    create_change(context, r1);

    int finish = context->functionident[r1];
    int newfi = context->functionident[r1 + 1];
    int flag = 1;
    context->msp = 0;
    while (context->macrofunction[newfi] != '\n')
    {
        if (mletter(context, context->macrofunction[newfi]))
        {
            flag = 1;
            for (i = 0; i < 30; i++)
            {
                str[i] = 0;
            }
            i = 0;
            while (mletter(context, context->macrofunction[newfi]) ||
                   mdigit(context, context->macrofunction[newfi]))
            {
                str[i++] = context->macrofunction[newfi++];
            }
            for (kp = r1 + 2; kp < finish;)
            {
                if (mequal(context, str, kp + 1))
                {
                    for (cp = context->functionident[kp];
                         context->fchange[cp] != '\n'; cp++)
                    {
                        context->mstring[context->msp++] = context->fchange[cp];
                    }
                    flag = 0;
                    break;
                }
                while (context->functionident[kp++] != 0)
                    ;
            }
            if (flag == 1)
            {
                for (i = 0; str[i] != 0; i++)
                {
                    context->mstring[context->msp++] = str[i];
                }
            }
        }
        else
        {
            context->mstring[context->msp++] = context->macrofunction[newfi];
            newfi++;
        }
    }
}

static int
scob(ruc_context *context, int cp)
{
    int i;
    context->fchange[cp++] = context->curchar;
    m_nextch(context, 6);
    while (context->curchar != EOF)
    {
        if (letter(context))
        {
            from_macrotext(context);
            for (i = 0; i < context->msp; i++)
            {
                context->fchange[cp++] = context->mstring[i];
            }
        }
        else if (context->curchar == '(')
        {
            cp = scob(context, cp);
        }
        else
        {
            context->fchange[cp++] = context->curchar;
            m_nextch(context, 6);
            if (context->curchar != ')')
            {
                context->fchange[cp++] = context->curchar;
                m_nextch(context, 6);
                return cp;
            }
        }
    }
    m_error(context, scob_not_clous);
    return cp;
}

static void
create_change(ruc_context *context, int r1)
{
    int i;
    int r = r1 + 2;
    int cp = 1;
    context->functionident[r] = cp;
    if (context->curchar == '(')
    {
        m_nextch(context, 11);
        while (context->curchar != EOF)
        {

            if (letter(context))
            {
                from_macrotext(context);
                for (i = 0; i < context->msp; i++)
                {
                    context->fchange[cp++] = context->mstring[i];
                }
            }
            else if (context->curchar == '(')
            {
                cp = scob(context, cp);
            }
            else if (context->curchar != ')' && context->curchar != ',')
            {
                context->fchange[cp++] = context->curchar;
                m_nextch(context, 11);
            }
            else
            {
                m_error(context, not_enough_param);
            }

            if (context->curchar == ',' || context->curchar == ')')
            {
                for (; context->functionident[r] != 0; r++)
                    ;

                if (r < context->functionident[r1])
                {
                    context->fchange[cp++] = '\n';
                    r++;
                }
                else
                {
                    m_error(context, not_enough_param);
                }

                if (context->curchar == ',')
                {
                    context->functionident[r] = cp;
                    m_nextch(context, 11);
                }
                else
                {
                    if (r != context->functionident[r1])
                    {
                        m_error(context, not_enough_param2);
                    }
                    m_nextch(context, 11);
                    return;
                }
            }
        }
        m_error(context, scob_not_clous);
    }
    else
    {
        m_error(context, stalpe);
    }
}

static void
r_macrofunction(ruc_context *context)
{
    int j;
    int olderfip = context->fip++;
    context->functionident[context->fip++] = context->mfp;

    to_functionident(context);
    m_nextch(context, 3);
    context->functionident[olderfip] = context->fip;

    while (context->curchar != '\n')
    {

        if (letter(context))
        {
            from_macrotext(context);
            for (j = 0; j < context->msp; j++)
            {
                context->macrofunction[context->mfp++] = context->mstring[j];
            }
            context->msp = 0;
        }
        else
        {
            context->macrofunction[context->mfp++] = context->curchar;
            m_nextch(context, 3);
        }

        if (context->curchar == EOF)
        {
            m_error(context, not_end_fail_preprocess);
        }
    }
    context->macrofunction[context->mfp++] = '\n';
    return;
}


/*void m_ident()
{
    context->msp = 0;

    while(letter(context) || digit(context))
    {
        context->mstring[context->msp++] = context->curchar;
        m_nextch(context, 5);
    }
    from_macrotext(context);

    return;
}*/

static int
find_ident(ruc_context *context)
{
    int fpr = context->rp;
    int i, r;
    context->hash = 0;
    fpr += 2;
    for (i = 0; i < context->msp; i++)
    {
        context->hash += context->mstring[i];
        context->reprtab[fpr++] = context->mstring[i];
    }
    context->reprtab[fpr++] = 0;
    context->hash &= 255;
    r = context->hashtab[context->hash];
    while (r)
    {
        if (r >= context->mfirstrp && r <= context->mlastrp &&
            equal(context, r, context->rp))
        {
            return r;
        }
        r = context->reprtab[r];
    }
    return 0;
}

static int
check_if(ruc_context *context, int type_if)
{

    int flag = 0;

    if (type_if == SH_IF)
    {
        m_error(context, not_relis_if);
    }
    else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
    {
        context->msp = 0;
        while (letter(context) || digit(context))
        {
            context->mstring[context->msp++] = context->curchar;
            m_nextch(context, 10);
        }
        if (find_ident(context))
        {
            flag = 1;
        }
        if (type_if == SH_IFDEF)
        {
            return flag;
        }
        else
        {
            return 1 - flag;
        }
    }
    return 0;
}

static void
end_line(ruc_context *context)
{
    while (context->curchar != '\n')
    {
        if (context->curchar == ' ' || context->curchar == '\t')
        {
            m_nextch(context, 9);
        }
        else
        {
            m_error(context, after_preproces_words_must_be_space);
        }
    }
    m_nextch(context, 9);
}

static void
false_if(ruc_context *context)
{
    int fl_cur;
    while (context->curchar != EOF)
    {
        if (context->curchar == '#')
        {
            fl_cur = macro_keywords(context);
            if (fl_cur == SH_ENDIF)
            {
                context->checkif--;
                if (context->checkif == -1)
                {
                    m_error(context, befor_endif);
                }
                return;
            }
            if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                context->checkif++;
                false_if(context);
            }
        }
        else
        {
            m_nextch(context, 8);
        }
    }
    m_error(context, must_be_endif);
}

static int
m_false(ruc_context *context)
{
    int fl_cur = context->cur;
    while (context->curchar != EOF)
    {
        if (context->curchar == '#')
        {
            fl_cur = macro_keywords(context);
            if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
            {
                return fl_cur;
            }
            if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                false_if(context);
            }
        }
        else
        {
            m_nextch(context, 7);
        }
    }
    m_error(context, must_be_endif);
    return 1;
}

static void
m_true(ruc_context *context, int type_if)
{
    while (context->curchar != EOF)
    {
        macroscan(context);
        if (context->cur == SH_ELSE || context->cur == SH_ELIF)
        {
            break;
        }
        if (context->cur == SH_ENDIF)
        {
            context->checkif--;
            if (context->checkif == -1)
            {
                m_error(context, befor_endif);
            }
            return;
        }
    }

    if (type_if != SH_IF && context->cur == SH_ELIF)
    {
        m_error(context, dont_elif);
    }

    false_if(context);
    return;
}

static void
m_if(ruc_context *context, int type_if)
{
    context->checkif++;
    int flag = check_if(context, type_if); // начало (if)
    end_line(context);
    if (flag)
    {
        m_true(context, type_if);
        return;
    }
    else
    {
        context->cur = m_false(context);
    }

    /*if (type_if == SH_IF) // середина (else if)
    {
        while (context->cur == SH_ELIF)
        {
            flag = check_if(type_if);
            end_line();
            if(flag)
            {
                m_true(context, type_if);
                return;
            }
            else
            {
                context->cur = m_folse();
            }

        }
    }
    else if (context->cur == SH_ELIF)
    {
        printf("Неправильное макрослово\n");
        exit (10);
    }*/

    if (context->cur == SH_ELSE) // конец (else)
    {
        context->cur = 0;
        m_true(context, type_if);
        return;
    }

    if (context->cur == SH_ENDIF)
    {
        context->checkif--;
        if (context->checkif == -1)
        {
            m_error(context, befor_endif);
        }
    }
}

static void
macroscan(ruc_context *context)
{
    int j;
    switch (context->curchar)
    {
        case EOF:
            return;

        case '#':
            context->cur = macro_keywords(context);
            context->prep_flag = 1;
            printer_printf(&context->miscout_options, "flag");
            if (context->cur == SH_DEFINE)
            {
                relis_define(context);
                m_nextch(context, 1);
                return;
            }

            else if (context->cur == SH_IF || context->cur == SH_IFDEF ||
                     context->cur == SH_IFNDEF)
            {
                m_if(context, context->cur);
                return;
            }
            else if (context->cur == SH_ELSE || context->cur == SH_ELIF ||
                     context->cur == SH_ENDIF)
            {
                return;
            }
            else
            {
                m_fprintf(context, context->curchar);
                m_nextch(context, 17);
                return;
                m_error(context, preproces_words_not_exist);
            }

        case '\'':
            m_fprintf(context, context->curchar);
            m_nextch(context, 171);
            if (context->curchar == '\\')
            {
                m_fprintf(context, context->curchar);
                m_nextch(context, 171);
            }
            m_fprintf(context, context->curchar);
            m_nextch(context, 171);

            m_fprintf(context, context->curchar);
            m_nextch(context, 171);
            return;

        case '\"':
            m_fprintf(context, context->curchar);
            m_nextch(context, 172);
            while (context->curchar != '\"' && context->curchar != EOF)
            {
                if (context->curchar == '\\')
                {
                    m_fprintf(context, context->curchar);
                    m_nextch(context, 172);
                }
                m_fprintf(context, context->curchar);
                m_nextch(context, 172);
            }
            m_fprintf(context, context->curchar);
            m_nextch(context, 172);
            return;
        default:
            if (letter(context))
            {
                from_macrotext(context);
                for (j = 0; j < context->msp; j++)
                {
                    m_fprintf(context, context->mstring[j]);
                }
                return;
            }
            else
            {
                m_fprintf(context, context->curchar);
                m_nextch(context, 173);
                return;
            }
    }
}

void
preprocess_file(ruc_context *context)
{
    context->mfirstrp = context->rp;
    context->mlines[context->mline = 1] = 1;
    context->charnum = 1;
    context->mcl = 1;

    getnext(context);
    m_nextch(context, 18);
    while (context->curchar != EOF)
    {
        macroscan(context);
    }
    context->m_conect_lines[context->mcl++] = context->mline - 1;
}
