#define _CRT_SECURE_NO_WARNINGS
#define MAXPARAMS 100
#define MAXPARAMLENGTH 100
#define MAXDEFSTAB 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"

extern int scanner();
extern int error(int);

int sc();
int check_condition();
int is_defined();
void must(int, int);

void print_lexem();
void process_create_def();
void process_if();
void skip_to_endif();
void apply_define();
void add_lexem_in_marco_body();
int find_repr_in_args(int arg_repr);
int getnext();
int letter();
int digit();
int equal(int, int);


int old_char = 1;
int old_def = 0;
int need_to_print_lexem = 1;
int curr_def = 0;
int params_repr[MAXPARAMS], pr = 0;
char params_values[MAXPARAMS][MAXPARAMLENGTH], pv = 0;

// type : 0 -- int, 1 -- float, 2 -- char, 3 -- string, 4 -- macro
// 1 запись: next_def_ref, repr, type, value, 0, 0 (без параметров)
// 1 запись: next_def_ref, repr, type, args_count, macro_body, 0, 0 (с параметрами)
int deftab[MAXDEFSTAB], df = 0;

void preprocess_file()
{
    int repeat = 1;
    while (repeat)
    {
        if (sc() == LEOF)
        {
            repeat = 0;
        }
        else if (cur == SH_DEFINE)
        {
            process_create_def();
        }
        else if (cur == SH_IFDEF || cur == SH_IFNDEF || cur == SH_IF)
        {
            process_if();
        }
        else if (cur == SH_ELSE || next == SH_ELIF || next == SH_ENDIF)
        {
            error(sh_if_not_found);
        }
        else if (next == IDENT && is_defined())
        {
            apply_define();
            sc();
            //printf("nextlexem = %i\n", sc());
        }
        else
        {
            print_lexem();
            printf("nextlexem = %i\n", sc());
        }
    }
}

void process_create_def()
{
    int i = 0, lexem_type = 0;
    int arg_number = 0;
    must(IDENT, no_ident_after_define); 
    deftab[df] = df ? old_def : -1;
    old_def = df++;
    deftab[df++] = repr;
    
    lexem_type = sc();
    if (lexem_type == NUMBER)
    {
        if (ansttype == LINT)
        {
            deftab[df++] = 0;
            deftab[df++] = num;
        }
        else if (ansttype == LFLOAT)
        {
            deftab[df++] = 1;
            deftab[df++] = numr.first;
            deftab[df++] = numr.second;

        }
        else if (ansttype == LCHAR)
        {
            deftab[df++] = 2;
            deftab[df++] = num;
        }
    }
    else if (lexem_type == STRING)
    {
        deftab[df++] = 3;
        while (lexstr[i] != 0)
        {
            deftab[df++] = lexstr[i];
            i++;
        }
        deftab[df] = 0;
    }
    else if (lexem_type == LEFTBR) // макрос
    {
        pr = 0;
        deftab[df++] = 4;
        
        // выедаем параметры, нам важно их число и ссылка на reprtab
        if (next == IDENT)
        {
            sc();
            params_repr[pr++] = repr;
            while (sc() == COMMA)
            {
                if (next == IDENT)
                {
                    sc();
                    params_repr[pr++] = repr;
                }
                else error(wait_ident_after_comma_in_macro_params); // не идент после запятой
            }
        }
        if (cur != RIGHTBR) error(wait_rightbr_in_macro_params);
        
        deftab[df++] = pr;
        
        while (curchar != '\n') // считаем выражение до конца строки
        {
            if (next == IDENT)
            {
                arg_number = find_repr_in_args(repr);
                if (arg_number != -1)
                {
                    deftab[df++] = '{';
                    deftab[df++] = '0' + arg_number;
                    deftab[df++] = '}';
                }
            }
            else
            {
                add_lexem_in_marco_body();
            }
            sc();
        }
        add_lexem_in_marco_body();
        /*for (int i = old_def + 4; i < df; i++)
         {
         printf("%c", deftab[i]);
         }*/
    }
    
    deftab[df++] = 0;
    deftab[df++] = 0;
}

int is_defined()
{
    int isdefined = 0;
    curr_def = old_def;
    
    while (curr_def != -1)
    {
        if (deftab[curr_def + 1] == repr)
        {
            isdefined = 1;
            break;
        }
        else
        {
            curr_def = deftab[curr_def];
        }
    }
    
    return isdefined;
}

void process_if()
{
    int repeat = 1;
    // true_condition значит, что мы находимся в ветке if, в которой верно условие (либо мы в if, и верно условие, либо в elif, и верно условие, либо в else)
    int true_condition = 0;
    int else_exists = 0; // после #else не должно быть #elseif
    
    true_condition = check_condition();
    sc();
    
    while (repeat)
    {
        if (next == SH_IFDEF || next == SH_IFNDEF || next == SH_IF) // вложенный if
        {
            sc();
            if (!true_condition)// зашли в новый #if в ветке с неверным условием
            {
                skip_to_endif();
            }
            else
            {
                process_if();
            }
            continue;
        }
        else if (next == SH_ELIF) // #elif
        {
            if (else_exists)
            {
                error(else_after_elif);
            }
            if (true_condition) // условие в #if или в #elif уже было выполнено => пропускаем все до нужного #endif
            {
                skip_to_endif();
                break;
            }
            else
            {
                sc();
                true_condition = check_condition();
                continue;
            }
        }
        else if (next == SH_ELSE) // #else
        {
            else_exists = 1;
            if (true_condition) // условие в #if или в #elif уже было выполнено => пропускаем все до нужного #endif
            {
                skip_to_endif();
                break;
            }
            else
            {
                true_condition = 1;
            }
        }
        else if (next == SH_ENDIF) // #endif -- конец условной компиляции
        {
            need_to_print_lexem = 1;
            sc();
            break;
        }
        else if (next == LEOF) // #ошибка -- дошли до конца, и не закрыли if
        {
            error(endif_not_found);
        }
        else if (true_condition) // обычная лексема -- печатаем или нет в зависимости от need_to_print_lexem
        {
            if (next == IDENT && is_defined())
            {
                apply_define();
            }
            else
            {
                print_lexem();
            }
        }
        sc();
    }
}

void skip_to_endif()
{
    int not_closed_if_count = 1;
    while (not_closed_if_count != 0)
    {
        if (next == SH_ENDIF) // #endif -- конец условной компиляции
        {
            not_closed_if_count--;
        }
        if (next == SH_IF || next == SH_IFDEF || next == SH_IFNDEF) // #endif -- в часть, которую нужно пропустить, попал #if. Теперь надо получить на 1 #endif больше
        {
            not_closed_if_count++;
        }
        else if (next == LEOF) // #ошибка -- дошли до конца, и не закрыли if
        {
            error(endif_not_found);
        }
        sc();
    }
    need_to_print_lexem = 1;
}

int check_condition()
{
    int res = 0;
    if (cur == SH_IFDEF)
    {
        return  is_defined();
    }
    else if (cur == SH_IFNDEF)
    {
        return !is_defined();
    }
    else if (cur == SH_IF || cur == SH_ELIF)
    {
    }
    return res;
}

void apply_define()
{
    int current_macro_body_char = 0, pv_curchar = 0;
    int define_type = 0, i = 0, c = 0;
    int currect_param_num = 0;
    int macro_body_start_index = curr_def + 4;
    int val = 0;
    float fval = 0.0;
    pv = 0;
    define_type = deftab[curr_def + 2];
    val = deftab[curr_def + 3];
    switch (define_type)
    {
        case 0:
            fprintf(output, "%d", val);
            break;
        case 1:
            memcpy(&fval, &val, sizeof(int));
            fprintf(output, "%f", fval);
            break;
        case 2:
            fprintf(output, "%c", '\'');
            fprintf(output, "%c", val);
            fprintf(output, "%c", '\'');
            break;
        case 3:
            fprintf(output, "%c", '"');
            while ((c = deftab[curr_def + 3 + i]) != 0)
            {
                fprintf(output, "%c", c);
                i++;
            }
            fprintf(output, "%c", '"');
            break;
        case 4:
            sc();
            must(LEFTBR, macro_params_not_found);
            
            while (next != RIGHTBR)
            {
                if (next == COMMA)
                {
                    pv++;
                    params_values[pv][pv_curchar] = 0;
                    pv_curchar = 0;
                    sc();
                }
                // копируем параметр
                for (i = old_char; i < charnum - 1; i++)
                {
                    params_values[pv][pv_curchar++] = source[i];
                }		
                sc();
            }
            
            pv++; // не посчитали 1 параметр
            if (pv != deftab[curr_def + 3])
            {
                error(params_count_not_equals_in_macro); // количество параметров не совпадает
            }
            if (next != RIGHTBR) error(wait_rightbr_in_macro_params); // нет закрывающей скобки
            
            i = 0;
            while ((current_macro_body_char = deftab[macro_body_start_index + i]) != 0)
            {
                if (current_macro_body_char == '{' && deftab[macro_body_start_index + i + 2] == '}')
                {
                    currect_param_num = deftab[macro_body_start_index + i + 1] - '0';
                    printf("%s", params_values[currect_param_num]);
                    i += 2;
                }
                else
                {
                    printf("%c", current_macro_body_char);
                }
                i++;
            }
            val = 1;
            break;
    }
}

void print_lexem()
{
    int i;
    for (i = old_char; i < charnum-1; i++)
    {
        printf("source[%i] = '%c'\n", i, source[i]);
        fprintf(output, "%c", source[i]);
    }
    old_char = charnum-1;
}

void add_lexem_in_marco_body()
{
    int i;
    for (i = old_char; i < charnum-1; i++)
    {
        deftab[df++] = source[i];
    }
    old_char = charnum-1;
}

int find_repr_in_args(int arg_repr)
{
    int i, res = pr;
    for (i = 0; i < pr; i++)
    {
        if (arg_repr == params_repr[i])
        {
            res = i;
            break;
        }
    }
    return res < pr ? res : -1;
}

int sc()
{
    curchar = getnext();
    if (letter() || curchar == '#')
    {
        int oldrepr, r;
        oldrepr = rp;
        rp+=2;
        hash = 0;
        
        do
        {
            
            hash += curchar;
            reprtab[rp++] = curchar;
            curchar = getnext();
        }
        while (letter() || digit());
        
        hash &= 255;
        reprtab[rp++] = 0;
        r = hashtab[hash];
        if (r)
        {
            do
            {
                if (equal(r, oldrepr))
                {
                    rp = oldrepr;
                    return (reprtab[r+1] < 0) ? reprtab[r+1] : (repr = r, IDENT);
                }
                else
                    r = reprtab[r];
            }
            while (r);
        }
        reprtab[oldrepr] = hashtab[hash];
        repr = hashtab[hash] = oldrepr;
        reprtab[repr+1] = (keywordsnum) ? -((++keywordsnum - 2)/4) : 1;  // 0 - только MAIN, < 0 - ключевые слова, 1 - обычные иденты
        return IDENT;
    }

    old_char = charnum - 1;
    return scanner();
}

void must(int what, int e)
{
    if (sc() != what)
        error(e);
}
