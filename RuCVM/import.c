//
//  import.c
//
//  Created by Andrey Terekhov on 2/25/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

//#define ROBOT
#include <unistd.h>
#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "context.h"
#include "th_static.h"

// Я исхожу из того, что нумерация нитей процедурой t_create начинается с 1 и
// идет последовательно в соответствии с порядком вызовов этой процудуры,
// главная программа имеет номер 0. Если стандарт POSIX этого не обеспечивает,
// это должен сделать Саша Головань.

// Память mem, как обычно, начинается с кода всей программы, включая нити, затем
// идут глобальные данные, затем куски для стеков и массивов гланой программы и
// нитей, каждый кусок имеет размер MAXMEMTHREAD.

// Поскольку и при запуске главной прграммы, и при запуске любой нити процудура
// t_create получает в качестве параметра одну и ту же процедуру interpreter,
// важно, чтобы при начале работы программы или нити были правильно установлены
// l, x и pc, причем все важные переменные были локальными, тогда дальше все
// переключения между нитями будут заботой ОС.

// Есть глобальный массив context->threads, i-ый элемент которого указывает на
// начало куска i-ой нити. Каждый кусок начинается с шапки, где хранятся l, x и
// pc, которые нужно установить в момент старта нити.

#include "Defs.h"

#define I2CBUFFERSIZE 50

#define index_out_of_range 1
#define wrong_kop 2
#define wrong_arr_init 3
#define wrong_motor_num 4
#define wrong_motor_pow 5
#define wrong_digsensor_num 6
#define wrong_ansensor_num 7
#define wrong_robot_com 8
#define wrong_number_of_elems 9
#define zero_devide 10
#define float_zero_devide 11
#define mem_overflow 12
#define sqrt_from_negat 13
#define log_from_negat 14
#define log10_from_negat 15
#define wrong_asin 16
#define wrong_string_init 17
#define printf_runtime_crash 18
#define init_err 19

char sem_print[] = "sem_print", sem_debug[] = "sem_debug";

int
szof(vm_context *context, int type)
{
    return context->modetab[type] == MARRAY
        ? 1
        : type == LFLOAT ? 2
                         : (type > 0 && context->modetab[type] == MSTRUCT)
                ? context->modetab[type + 1]
                : 1;
}

void
runtimeerr(vm_context *context, int e, int i, int r)
{
    switch (e)
    {
        case index_out_of_range:
            printer_printf(&context->error_options,
                           "индекс %i за пределами границ массива %i\n", i,
                           r - 1);
            break;
        case wrong_kop:
            printer_printf(&context->error_options,
                           "команду %i я пока не реализовал; номер нити = %i\n",
                           i, r);
            break;
        case wrong_arr_init:
            printer_printf(
                &context->error_options,
                "массив с %i элементами инициализируется %i значениями\n", i,
                r);
            break;
        case wrong_string_init:
            printer_printf(
                &context->error_options,
                "строковая переменная с %i элементами инициализируется "
                "строкой с %i литерами\n",
                i, r);
            break;
        case wrong_motor_num:
            printer_printf(
                &context->error_options,
                "номер силового мотора %i, а должен быть от 1 до 4\n", i);
            break;
        case wrong_motor_pow:
            printer_printf(
                &context->error_options,
                "задаваемая мощность мотора %i равна %i, а должна быть от "
                "-100 до 100\n",
                i, r);
            break;
        case wrong_digsensor_num:
            printer_printf(
                &context->error_options,
                "номер цифрового сенсора %i, а должен быть 1 или 2\n", i);
            break;
        case wrong_ansensor_num:
            printer_printf(
                &context->error_options,
                "номер аналогового сенсора %i, а должен быть от 1 до 6\n", i);
            break;
        case wrong_robot_com:
            printer_printf(&context->error_options,
                           "робот не может исполнить команду\n");
            break;
        case wrong_number_of_elems:
            printer_printf(
                &context->error_options,
                "количество элементов в массиве по каждому измерению должно "
                "быть положительным, а тут %i\n",
                r);
            break;
        case zero_devide:
            printer_printf(&context->error_options, "целое деление на 0\n");
            break;
        case float_zero_devide:
            printer_printf(&context->error_options,
                           "вещественное деление на 0\n");
            break;
        case mem_overflow:
            printer_printf(
                &context->error_options,
                "переполнение памяти, скорее всего, нет выхода из рекурсии\n");
            break;
        case sqrt_from_negat:
            printer_printf(
                &context->error_options,
                "попытка вычисления квадратного корня из отрицательного "
                "числа \n");
            break;
        case log_from_negat:
            printer_printf(&context->error_options,
                           "попытка вычисления натурального логарифма из 0 или "
                           "отрицательного числа\n");
            break;
        case log10_from_negat:
            printer_printf(&context->error_options,
                           "попытка вычисления десятичного логарифма из 0 или "
                           "отрицательного числа\n");
            break;
        case wrong_asin:
            printer_printf(
                &context->error_options,
                "аргумент арксинуса должен быть в отрезке [-1, 1]\n");
            break;

        case printf_runtime_crash:
            printer_printf(
                &context->error_options,
                "странно, printf не работает на этапе исполнения; ошибка "
                "коммпилятора");
            break;
        case init_err:
            printer_printf(
                &context->error_options,
                "количество элементов инициализации %i не совпадает с "
                "количеством элементов %i массива\n",
                i, r);
            break;

        default:;
    }
    exit(3);
}


#ifdef ROBOT
FILE *      f1, *f2; // файлы цифровых датчиков
const char *JD1 = "/sys/devices/platform/da850_trik/sensor_d1";
const char *JD2 = "/sys/devices/platform/da850_trik/sensor_d2";

int
rungetcommand(vm_context *context, const char *command)
{
    FILE *fp;
    long  x = -1;
    char  path[100] = { '\0' };

    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL)
        runtimeerr(context, wrong_robot_com, 0, 0);

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        x = strtol(path, NULL, 16);
        printer_printf(&context->output_options, "[%s] %ld\n", path, x);
    }
    pclose(fp);
    return x; // ??????
}

#endif

/*
void prmem()
{
    int i;
    printf("mem=\n");
    for (i=context->g; i<=x; i++)
        printf("%i ) %i\n",i, context->mem[i]);
    printf("\n");

}
*/

void
auxprintf(vm_context *context, int strbeg, int databeg)
{

    int i, j, curdata = databeg + 1;
    for (i = strbeg; context->mem[i] != 0; ++i)
    {
        if (context->mem[i] == '%')
        {
            switch (context->mem[++i])
            {
                case 'i':
                case 1094: // ц
                    printer_printf(&context->output_options, "%i",
                                   context->mem[curdata++]);
                    break;

                case 'c':
                case 1083: // л
                    printer_printchar(&context->output_options,
                                      context->mem[curdata++]);
                    break;

                case 'f':
                case 1074: // в
                    printer_printf(&context->output_options, "%lf",
                                   *((double *)(&context->mem[curdata])));
                    curdata += 2;
                    break;

                case 's':
                case 1089: // с
                    for (j = context->mem[curdata]; j - context->mem[curdata] <
                         context->mem[context->mem[curdata] - 1];
                         ++j)
                        printer_printchar(&context->output_options,
                                          context->mem[j]);
                    curdata++;
                    break;

                case '%':
                    printer_printf(&context->output_options, "%%");
                    break;

                default:

                    break;
            }
        }
        else
        {
            printer_printchar(&context->output_options, context->mem[i]);
        }
    }
}

void
auxprint(vm_context *context, int beg, int t, char before, char after)
{
    double rf;
    int    r;
    r = context->mem[beg];
    if (before)
    {
        printer_printf(&context->output_options, "%c", before);
    }

    if (t == LINT)
    {
        printer_printf(&context->output_options, "%i", r);
    }
    else if (t == LCHAR)
    {
        printer_printchar(&context->output_options, r);
    }
    else if (t == LFLOAT)
    {
        memcpy(&rf, &context->mem[beg], sizeof(double));
        printer_printf(&context->output_options, "%20.15f", rf);
    }
    else if (t == LVOID)
    {
        printer_printf(&context->error_options,
                       " значения типа ПУСТО печатать нельзя\n");
    }

    // здесь t уже точно положительный
    else if (context->modetab[t] == MARRAY)
    {
        int rr = r, i, type = context->modetab[t + 1], d;
        d = szof(context, type);

        if (type > 0)
            for (i = 0; i < context->mem[rr - 1]; i++)
                auxprint(context, rr + i * d, type, 0, '\n');
        else
            for (i = 0; i < context->mem[rr - 1]; i++)
                auxprint(context, rr + i * d, type, 0,
                         (type == LCHAR ? 0 : ' '));
    }
    else if (context->modetab[t] == MSTRUCT)
    {
        int cnt = context->modetab[t + 2], i;
        printer_printf(&context->output_options, "{");
        for (i = 2; i <= cnt; i += 2)
        {
            int type = context->modetab[t + i + 1];
            if (type < 0)
                auxprint(context, beg, type, (i == 2 ? 0 : ' '),
                         (i == cnt ? 0 : ','));
            else
                auxprint(context, beg, type, '\n', '\n');
            beg += szof(context, type);
        }
        printer_printf(&context->output_options, "}");
    }
    else
    {
        printer_printf(&context->error_options,
                       " значения типа ФУНКЦИЯ и указателей печатать нельзя\n");
    }

    if (after)
        printer_printf(&context->output_options, "%c", after);
}


void
auxget(vm_context *context, int beg, int t)
{
    double rf;
    //     printf("beg=%i t=%i\n", beg, t);
    if (t == LINT)
    {
        scanner_scanf(&context->input_options, " %i", &context->mem[beg]);
    }
    else if (t == LCHAR)
    {
        context->mem[beg] = scanner_getnext(&context->input_options);
    }
    else if (t == LFLOAT)
    {
        scanner_scanf(&context->input_options, " %lf", &rf);
        memcpy(&context->mem[beg], &rf, sizeof(double));
    }
    else if (t == LVOID)
    {
        printer_printf(&context->error_options,
                       " значения типа ПУСТО вводить нельзя\n");
    }

    // здесь t уже точно положительный
    else if (context->modetab[t] == MARRAY)
    {
        int rr = context->mem[beg], i, type = context->modetab[t + 1], d;
        d = szof(context, type);
        for (i = 0; i < context->mem[rr - 1]; i++)
            auxget(context, rr + i * d, type);
    }
    else if (context->modetab[t] == MSTRUCT)
    {
        int cnt = context->modetab[t + 2], i;
        for (i = 2; i <= cnt; i += 2)
        {
            int type = context->modetab[t + i + 1];
            auxget(context, beg, type);
            beg += szof(context, type);
        }
    }
    else
        printer_printf(&context->error_options,
                       " значения типа ФУНКЦИЯ и указателей вводить нельзя\n");
}

void *interpreter(void *);

int
check_zero_int(vm_context *context, int r)
{
    if (r == 0)
        runtimeerr(context, zero_devide, 0, 0);
    return r;
}

double
check_zero_float(vm_context *context, double r)
{
    if (r == 0)
        runtimeerr(context, float_zero_devide, 0, 0);
    return r;
}

int
dsp(vm_context *context, int di, int l)
{
    return di < 0 ? context->g - di : l + di;
}

void *
invoke_interpreter(vm_context *context, void *arg)
{
    ruc_vm_thread_arg tharg;
    tharg.context = context;
    tharg.arg = arg;
    return interpreter(&tharg);
}

void *
interpreter(void *thread_arg)
{
    ruc_vm_thread_arg *arg = (ruc_vm_thread_arg *)thread_arg;
    vm_context *       context = arg->context;
    int    l, x, origpc = *((int *)arg->arg), numTh = t_getThNum(context);
    int    N, bounds[100], d, from, prtype, cur0, pc = abs(origpc);
    int    i, r, flagstop = 1, entry, di, di1, len;
    int    num;
    int    a_str1;
    int    str1;
    int    str2;
    double lf, rf;


    if (origpc > 0)
    {
        if (numTh)
        {
            context->threads[numTh] = cur0 = numTh * MAXMEMTHREAD;
            l = context->mem[context->threads[numTh]] =
                context->threads[numTh] + 2;
            x = context->mem[context->threads[numTh]] =
                l + context->mem[pc - 2]; // l + maxdispl
            context->mem[l + 2] = -1;
        }
        else
        {
            l = context->threads[0] + 2;
            x = context->mem[context->threads[0] + 1];
        }
    }
    else
    {
        l = context->mem[context->threads[numTh]];
        x = context->mem[context->threads[numTh] + 1];
    }
    flagstop = 1;
    while (flagstop)
    {
        memcpy(&rf, &context->mem[x - 1], sizeof(double));
        // printf("pc=%i context->mem[pc]=%i\n", pc, context->mem[pc]);
        // printf("running th #%i\n", t_getThNum());

        switch (context->mem[pc++])
        {
            case STOP:
                flagstop = 0;
                context->xx = x;
                break;

            case CREATEDIRECTC:
            {
                i = pc;

                context->mem[++x] =
                    t_create_inner(context, interpreter, (void *)&i);
                break;
            }
            case CREATEC:
            {
                int i;

                i = context->mem[x];
                entry = context->functions[i > 0 ? i : context->mem[l - i]];
                i = entry + 3; // новый pc
                context->mem[x] =
                    t_create_inner(context, interpreter, (void *)&i);
            }
            break;

            case JOINC:
                t_join(context, context->mem[x--]);
                break;

            case SLEEPC:
                t_sleep(context, context->mem[x--]);
                break;

            case EXITDIRECTC:
            case EXITC:
                t_exit(context);
                break;

            case SEMCREATEC:
                context->mem[x] = t_sem_create(context, context->mem[x]);
                break;

            case SEMPOSTC:
                t_sem_post(context, context->mem[x--]);
                break;

            case SEMWAITC:
                t_sem_wait(context, context->mem[x--]);
                break;

            case INITC:
                t_init(context);
                break;

            case DESTROYC:
                t_destroy(context);
                break;

            case MSGRECEIVEC:
            {
                struct msg_info m = t_msg_receive(context);
                context->mem[++x] = m.numTh;
                context->mem[++x] = m.data;
            }
            break;

            case MSGSENDC:
            {
                struct msg_info m;
                m.data = context->mem[x--];
                m.numTh = context->mem[x--];
                t_msg_send(context, m);
            }
            break;

            case GETNUMC:
                context->mem[++x] = numTh;
                break;

            case WIFI_CONNECTC:
                break;
            case BLYNK_AUTHORIZATIONC:
                break;
            case BLYNK_SENDC:
                break;
            case BLYNK_RECEIVEC:
                break;
            case BLYNK_NOTIFICATIONC:
                break;
            case BLYNK_PROPERTYC:
                break;
            case BLYNK_LCDC:
                break;
            case BLYNK_TERMINALC:
                break;

            case SETSIGNALC:
                break;
            case PIXELC:
                break;
            case LINEC:
                break;
            case RECTANGLEC:
                break;
            case ELLIPSEC:
                break;
            case CLEARC:
                break;
            case DRAW_STRINGC:
                break;
            case DRAW_NUMBERC:
                break;
            case ICONC:
                break;

#ifdef ROBOT

            case SETMOTORC:
            {
                int n, r;
                r = context->mem[x--];
                n = context->mem[x--];
                if (n < 1 || n > 4)
                    runtimeerr(context, wrong_motor_num, n, 0);
                if (r < -100 || r > 100)
                    runtimeerr(context, wrong_motor_pow, n, r);
                memset(i2ccommand, '\0', I2CBUFFERSIZE);
                printer_printf(&context->output_options,
                               "i2cset -y 2 0x48 0x%x 0x%x b\n", 0x14 + n - 1,
                               r);
                snprintf(i2ccommand, I2CBUFFERSIZE,
                         "i2cset -y 2 0x48 0x%x 0x%x b", 0x14 + n - 1, r);
                system(i2ccommand);
            }
            break;

            case GETDIGSENSORC:
            {
                int n = context->mem[x];
                if (n < 1 || n > 2)
                    runtimeerr(context, wrong_digsensor_num, n, 0);
                if (n == 1)
                    fscanf(f1, "%i", &i);
                else
                    fscanf(f2, "%i", &i);
                context->mem[x] = i;
            }
            break;

            case GETANSENSORC:
            {
                int n = context->mem[x];
                if (n < 1 || n > 6)
                    runtimeerr(context, wrong_ansensor_num, n, 0);
                memset(i2ccommand, '\0', I2CBUFFERSIZE);
                printer_printf(&context->output_options,
                               "i2cget -y 2 0x48 0x%x\n", 0x26 - n);
                snprintf(i2ccommand, I2CBUFFERSIZE, "i2cget -y 2 0x48 0x%x",
                         0x26 - n);
                context->mem[x] = rungetcommand(context, i2ccommand);
            }
            break;
#endif
            case FUNCBEG:
                pc = context->mem[pc + 1];
                break;
            case PRINT:
            {
                int t;
                sem_wait(context->sempr);
                t = context->mem[pc++];
                x -= szof(context, t);
                auxprint(context, x + 1, t, 0, '\n');
                fflush(stdout);
                sem_post(context->sempr);
            }
            break;
            case PRINTID:
                sem_wait(context->sempr);
                i = context->mem[pc++]; // ссылка на identtab
                prtype = context->identab[i + 2];
                r = context->identab[i + 1] + 2; // ссылка на reprtab
                do
                    printer_printchar(&context->output_options,
                                      context->reprtab[r++]);
                while (context->reprtab[r] != 0);

                if (prtype > 0 && context->modetab[prtype] == MARRAY &&
                    context->modetab[prtype + 1] > 0)
                    auxprint(context, dsp(context, context->identab[i + 3], l),
                             prtype, '\n', '\n');
                else
                    auxprint(context, dsp(context, context->identab[i + 3], l),
                             prtype, ' ', '\n');
                fflush(stdout);
                sem_post(context->sempr);
                break;

            /* Ожидает указатель на форматную строку на верхушке стека
             * Принимает единственным параметром суммарный размер того, что
             * нужно напечатать Проверок на типы не делает, этим занимался
             * компилятор Если захотим передавать динамически формируемые
             * строки, нужно будет откуда-то брать весь набор типов печатаемого
             */
            case PRINTF:
            {
                int sumsize, strbeg;
                sem_wait(context->sempr);
                sumsize = context->mem[pc++];
                strbeg = context->mem[x--];
                auxprintf(context, strbeg, x -= sumsize);
                fflush(stdout);
                sem_post(context->sempr);
            }
            break;
            case GETID:
                sem_wait(context->sempr);
                i = context->mem[pc++]; // ссылка на identtab
                prtype = context->identab[i + 2];
                r = context->identab[i + 1] + 2; // ссылка на reprtab
                do
                    printer_printchar(&context->output_options,
                                      context->reprtab[r++]);
                while (context->reprtab[r] != 0);
                printer_printf(&context->output_options, " ");
                fflush(stdout);

                auxget(context, dsp(context, context->identab[i + 3], l),
                       prtype);
                sem_post(context->sempr);
                break;
            case ABSIC:
                context->mem[x] = abs(context->mem[x]);
                break;
            case ABSC:
                rf = fabs(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case SQRTC:
                if (rf < 0)
                    runtimeerr(context, sqrt_from_negat, 0, 0);
                rf = sqrt(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case EXPC:
                rf = exp(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case SINC:
                rf = sin(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case COSC:
                rf = cos(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case LOGC:
                if (rf <= 0)
                    runtimeerr(context, log_from_negat, 0, 0);
                rf = log(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case LOG10C:
                if (rf <= 0)
                    runtimeerr(context, log10_from_negat, 0, 0);
                rf = log10(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case ASINC:
                if (rf < -1 || rf > 1)
                    runtimeerr(context, wrong_asin, 0, 0);
                rf = asin(rf);
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case RANDC:
                rf = (double)rand() / RAND_MAX;
                memcpy(&context->mem[++x], &rf, sizeof(double));
                ++x;
                break;
            case ROUNDC:
                context->mem[--x] = rf < 0 ? (int)(rf - 0.5) : (int)(rf + 0.5);
                break;

            case STRCPYC:
                str2 = context->mem[x--];
                a_str1 = context->mem[x--];
                context->mem[a_str1] = str2;
                break;
            case STRNCPYC:
                num = context->mem[x--];
                str2 = context->mem[x--];
                a_str1 = context->mem[x--];
                if (num > context->mem[str2 - 1])
                    exit(2); // erorr
                if (num <= context->mem[context->mem[a_str1] - 1])
                {
                    a_str1 = context->mem[a_str1];
                    context->mem[a_str1 - 1] = num;
                    num += a_str1;
                    while (a_str1 < num)
                        context->mem[a_str1++] = context->mem[str2++];
                }
                context->mem[x++] = num;
                context->mem[a_str1] = x;

                num += x;
                while (x < num)
                    context->mem[x++] = context->mem[str2++];
                x--;
                break;
            case STRCATC:
                str2 = context->mem[x--];
                a_str1 = context->mem[x--];
                str1 = context->mem[a_str1];

                context->mem[x++] = context->mem[str2 - 1] +
                    context->mem[context->mem[a_str1] - 1];
                context->mem[a_str1] = x;
                a_str1 = context->mem[a_str1];

                num = x + context->mem[str1 - 1];
                while (x < num)
                    context->mem[x++] = context->mem[str1++];

                num = x + context->mem[str2 - 1];
                while (x < num)
                    context->mem[x++] = context->mem[str2++];
                x--;

                break;
            case STRNCATC:
                num = context->mem[x--];
                str2 = context->mem[x--];
                a_str1 = context->mem[x--];
                str1 = context->mem[a_str1];

                context->mem[x++] =
                    num + context->mem[context->mem[a_str1] - 1];
                context->mem[a_str1] = x;
                a_str1 = context->mem[a_str1];

                i = x + context->mem[str1 - 1];
                while (x < i)
                    context->mem[x++] = context->mem[str1++];

                num += x;
                while (x < num)
                    context->mem[x++] = context->mem[str2++];
                x--;
                break;
            case STRCMPC:
                str2 = context->mem[x--];
                a_str1 = context->mem[x];
                if (context->mem[a_str1 - 1] < context->mem[str2 - 1])
                {
                    context->mem[x] = 1;
                    break;
                }
                else if (context->mem[a_str1 - 1] > context->mem[str2 - 1])
                {
                    context->mem[x] = -1;
                    break;
                }
                else
                {
                    for (i = 0; i < context->mem[str2 - 1]; i++)
                    {
                        if (context->mem[a_str1 + i] < context->mem[str2 + i])
                        {
                            context->mem[x] = 1;
                            break;
                        }
                        else if (context->mem[a_str1 + i] >
                                 context->mem[str2 + i])
                        {
                            context->mem[x] = -1;
                            break;
                        }
                    }
                    if (i == context->mem[str2 - 1])
                    {
                        context->mem[x] = 0;
                    }
                }
                break;
            case STRNCMPC:
                num = context->mem[x--];
                str2 = context->mem[x--];
                a_str1 = context->mem[x];

                if (context->mem[a_str1 - 1] < context->mem[str2 - 1] &&
                    context->mem[a_str1 - 1] < num)
                {
                    context->mem[x] = 1;
                    break;
                }
                else if (context->mem[a_str1 - 1] > context->mem[str2 - 1] &&
                         context->mem[str2 - 1] < num)
                {
                    context->mem[x] = -1;
                    break;
                }
                else
                {
                    for (i = 0; i < num; i++)
                    {
                        if (i == context->mem[a_str1 - 1] &&
                            i == context->mem[str2 - 1])
                        {
                            context->mem[x] = 0;
                            break;
                        }

                        if (context->mem[a_str1 + i] < context->mem[str2 + i])
                        {
                            context->mem[x] = 1;
                            break;
                        }
                        if (context->mem[a_str1 + i] > context->mem[str2 + i])
                        {
                            context->mem[x] = -1;
                            break;
                        }
                    }
                    if (i == num)
                        context->mem[x] = 0;
                }
                break;
            case STRSTRC:
            {
                int j, flag = 0;
                str2 = context->mem[x--];
                a_str1 = context->mem[x];
                for (i = 0;
                     i < context->mem[a_str1 - 1] - context->mem[str2 - 1]; i++)
                {
                    if (context->mem[str2] == context->mem[a_str1 + i])
                    {
                        for (j = 0; j < context->mem[str2 - 1]; j++)
                        {
                            if (context->mem[str2 + j] !=
                                context->mem[a_str1 + i + j])
                            {
                                flag = 1;
                                break;
                            }
                        }
                        if (flag == 0)
                        {
                            context->mem[x] = i + 1;
                            break;
                        }
                        flag = 0;
                    }
                }
                if (i >= context->mem[a_str1 - 1] - context->mem[str2 - 1])
                {

                    context->mem[x] = -1;
                    break;
                }

                break;
            }
            case STRLENC:
                a_str1 = context->mem[x];
                context->mem[x] = context->mem[a_str1 - 1];
                break;
            case STRUCTWITHARR:
            {
                int oldpc, oldbase = context->base, procnum;
                context->base = dsp(context, context->mem[pc++], l);
                procnum = context->mem[pc++];
                oldpc = pc;
                pc = -procnum;
                context->mem[context->threads[numTh] + 1] = x;
                invoke_interpreter(context, (void *)&pc);
                x = context->xx;
                pc = oldpc;
                context->base = oldbase;
                flagstop = 1;
            }
            break;
            case DEFARR: // N, d, displ, proc     на стеке N1, N2, ... ,
                         // context->NN
            {
                int N = context->mem[pc++];
                int d = context->mem[pc++];
                int curdsp = context->mem[pc++];
                int proc = context->mem[pc++];
                int usual = context->mem[pc++];
                int all = context->mem[pc++];
                int instruct = context->mem[pc++];

                int stackC0[10], stacki[10], i, curdim = 1;
                if (usual >= 2)
                    usual -= 2;
                context->NN = context->mem[x]; // будет использоваться в ARRINIT
                                               // только при usual=1
                for (i = usual && all ? N + 1 : N; i > 0; i--)
                    if ((bounds[i] = context->mem[x--]) <= 0)
                        runtimeerr(context, wrong_number_of_elems, 0,
                                   bounds[i]);
                if (N > 0)
                {
                    stacki[1] = 0;
                    context->mem[++x] = bounds[1];
                    context->mem[instruct ? context->base + curdsp
                                          : dsp(context, curdsp, l)] =
                        stackC0[1] = x + 1;
                    x += bounds[1] * (curdim < abs(N) ? 1 : d);

                    if (x >= context->threads[numTh] + MAXMEMTHREAD)
                        runtimeerr(context, mem_overflow, 0, 0);

                    if (N == 1)
                    {
                        if (proc)
                        {
                            int curx = x, oldbase = context->base, oldpc = pc,
                                i;
                            for (i = stackC0[1]; i <= curx; i += d)
                            {
                                pc = -proc; // вычисление границ очередного
                                            // массива в структуре
                                context->base = i;
                                context->mem[context->threads[numTh] + 1] = x;
                                invoke_interpreter(context, (void *)&pc);
                                flagstop = 1;
                                x = context->xx;
                            }
                            pc = oldpc;
                            context->base = oldbase;
                        }
                    }
                    else
                    {
                        do
                        {
                            do
                            {
                                // go down
                                context->mem[++x] = bounds[curdim + 1];
                                context
                                    ->mem[stackC0[curdim] + stacki[curdim]++] =
                                    stackC0[curdim + 1] = x + 1;
                                x += bounds[curdim + 1] *
                                    (curdim == N - 1 ? d : 1);

                                if (x >= context->threads[numTh] + MAXMEMTHREAD)
                                    runtimeerr(context, mem_overflow, 0, 0);
                                ++curdim;
                                stacki[curdim] = 0;
                            } while (curdim < N);
                            // построена очередная вертикаль подмассивов

                            if (proc)
                            {
                                int curx = x, oldbase = context->base,
                                    oldpc = pc, i;
                                for (i = stackC0[curdim]; i <= curx; i += d)
                                {
                                    pc = proc; // вычисление границ очередного
                                               // массива в структуре
                                    context->base = i;
                                    context->mem[context->threads[numTh] + 1] =
                                        x;
                                    invoke_interpreter(context, (void *)&pc);
                                    flagstop = 1;
                                    x = context->xx;
                                }
                                pc = oldpc;
                                context->base = oldbase;
                            }
                            // go right
                            --curdim;
                        } while (stacki[curdim] < bounds[curdim]
                                     ? 1
                                     : /*up*/ curdim-- != N - 1);
                    }
                }
                context->adinit =
                    x + 1; // при usual == 1 использоваться не будет
            }
            break;
            case BEGINIT:
                context->mem[++x] = context->mem[pc++];
                break;
                //            case STRUCTINIT:
                //                pc++;
                //                break;
            case STRINGINIT:
                di = context->mem[pc++];
                r = context->mem[di < 0 ? context->g - di : l + di];
                N = context->mem[r - 1];
                from = context->mem[x--];
                d = context
                        ->mem[from - 1]; // d - кол-во литер в строке-инициаторе
                if (N != d)
                    runtimeerr(context, wrong_string_init, N, d);
                for (i = 0; i < N; i++)
                    context->mem[r + i] = context->mem[from + i];
                break;
            case ARRINIT:
                N = context->mem[pc++]; // N - размерность
                d = context->mem[pc++]; // d - шаг

                {
                    int addr = dsp(context, context->mem[pc++], l);
                    int usual = context->mem[pc++];
                    int onlystrings = usual >= 2 ? usual -= 2, 1 : 0;
                    int stA[10], stN[10], sti[10], stpnt = 1,
                                                   oldx = context->adinit;
                    if (N == 1)
                    {
                        if (onlystrings)
                            context->mem[addr] = context->mem[x--];
                        else
                        {
                            context->mem[addr] = context->adinit + 1;

                            if (usual &&
                                context->mem[context->adinit] !=
                                    context->NN) // здесь usual == 1,
                                                 // если usual == 0, проверка не
                                                 // нужна
                                runtimeerr(context, init_err,
                                           context->mem[context->adinit],
                                           context->NN);
                            context->adinit +=
                                context->mem[context->adinit] * d + 1;
                        }
                    }
                    else
                    {
                        stA[1] =
                            context->mem[addr]; // массив самого верхнего уровня
                        stN[1] = context->mem[stA[1] - 1];
                        sti[1] = 0;
                        if (context->mem[context->adinit] != stN[1])
                            runtimeerr(context, init_err,
                                       context->mem[context->adinit], stN[1]);
                        context->adinit++;
                        do
                        {

                            while (stpnt < N - 1)
                            {
                                stA[stpnt + 1] = context->mem[stA[stpnt]];
                                sti[++stpnt] = 0;
                                stN[stpnt] = context->mem[stA[stpnt] - 1];
                                if (context->mem[context->adinit] != stN[stpnt])
                                    runtimeerr(context, init_err,
                                               context->mem[context->adinit],
                                               stN[stpnt]);
                                context->adinit++;
                            }

                            do
                            {
                                if (onlystrings)
                                {
                                    context->mem[stA[stpnt] + sti[stpnt]] =
                                        context->mem[++oldx];
                                    if (usual &&
                                        context->mem[context->mem[oldx - 1] -
                                                     1] != context->NN)
                                        runtimeerr(
                                            context, init_err,
                                            context->mem[context->adinit],
                                            context->NN);
                                }
                                else
                                {
                                    if (usual &&
                                        context->mem[context->adinit] !=
                                            context->NN)
                                        runtimeerr(
                                            context, init_err,
                                            context->mem[context->adinit],
                                            context->NN);
                                    context->mem[stA[stpnt] + sti[stpnt]] =
                                        context->adinit + 1;
                                    context->adinit +=
                                        context->mem[context->adinit] * d + 1;
                                }
                            } while (++sti[stpnt] < stN[stpnt]);
                            if (stpnt > 1)
                            {
                                sti[stpnt] = 0;
                                stpnt--;
                                stA[stpnt] += ++sti[stpnt];
                            }
                        } while (stpnt != 1 || sti[1] != stN[1]);
                    }
                    x = context->adinit - 1;
                }
                break;

            case LI:
                context->mem[++x] = context->mem[pc++];
                break;
            case LID:
                memcpy(&context->mem[++x], &context->mem[pc++], sizeof(double));
                ++x;
                ++pc;
                break;
            case LOAD:
                context->mem[++x] =
                    context->mem[dsp(context, context->mem[pc++], l)];
                break;
            case LOADD:
                memcpy(&context->mem[++x],
                       &context->mem[dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                ++x;
                break;
            case LAT:
                context->mem[x] = context->mem[context->mem[x]];
                break;
            case LATD:
                memcpy(&rf, &context->mem[context->mem[x]], sizeof(double));
                memcpy(&context->mem[x++], &rf, sizeof(double));
                break;
            case LA:
                context->mem[++x] = dsp(context, context->mem[pc++], l);
                break;
            case CALL1:
                context->mem[l + 1] = ++x;
                context->mem[x++] = l;
                context->mem[x++] = 0; // следующая статика
                context->mem[x] = 0; // pc в момент вызова
                break;
            case CALL2:
                i = context->mem[pc++];
                entry = context->functions[i > 0 ? i : context->mem[l - i]];
                l = context->mem[l + 1];
                x = l + context->mem[entry + 1] - 1;
                if (x >= context->threads[numTh] + MAXMEMTHREAD)
                    runtimeerr(context, mem_overflow, 0, 0);
                context->mem[l + 2] = pc;
                pc = entry + 3;
                break;
            case RETURNVAL:
                d = context->mem[pc++];
                pc = context->mem[l + 2];
                if (pc == -1) // конец нити
                    flagstop = 0;
                else
                {
                    r = l;
                    l = context->mem[l];
                    context->mem[l + 1] = 0;
                    from = x - d;
                    x = r - 1;
                    for (i = 0; i < d; i++)
                        context->mem[++x] = context->mem[++from];
                }
                break;
            case RETURNVOID:
                pc = context->mem[l + 2];
                if (pc == -1) // конец нити
                    flagstop = 0;
                else
                {
                    x = l - 1;
                    l = context->mem[l];
                    context->mem[l + 1] = 0;
                }
                break;
            case NOP:;
                break;
            case B:
            case STRING:
                pc = context->mem[pc];
                break;
            case BE0:
                pc = (context->mem[x--]) ? pc + 1 : context->mem[pc];
                break;
            case BNE0:
                pc = (context->mem[x--]) ? context->mem[pc] : pc + 1;
                break;
            case SELECT:
                context->mem[x] += context->mem[pc++]; // ident displ
                break;
            case COPY00:
                di = dsp(context, context->mem[pc++], l);
                di1 = dsp(context, context->mem[pc++], l);
                len = context->mem[pc++];
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[di1 + i];
                break;
            case COPY01:
                di = dsp(context, context->mem[pc++], l);
                len = context->mem[pc++];
                di1 = context->mem[x--];
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[di1 + i];
                break;
            case COPY10:
                di = context->mem[x--];
                di1 = dsp(context, context->mem[pc++], l);
                len = context->mem[pc++];
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[di1 + i];
                break;
            case COPY11:
                di1 = context->mem[x--];
                di = context->mem[x--];
                len = context->mem[pc++];
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[di1 + i];
                break;
            case COPY0ST:
                di = dsp(context, context->mem[pc++], l);
                len = context->mem[pc++];
                for (i = 0; i < len; i++)
                    context->mem[++x] = context->mem[di + i];

                break;
            case COPY1ST:
                di = context->mem[x--];
                len = context->mem[pc++];
                for (i = 0; i < len; i++)
                    context->mem[++x] = context->mem[di + i];
                break;
            case COPY0STASS:
                di = dsp(context, context->mem[pc++], l);
                len = context->mem[pc++];
                x -= len;
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[x + i + 1];
                break;
            case COPY1STASS:
                len = context->mem[pc++];
                x -= len;
                di = context->mem[x--];
                for (i = 0; i < len; i++)
                    context->mem[di + i] = context->mem[x + i + 2];
                break;
            case COPYST:
                di = context->mem[pc++]; // смещ поля
                len = context->mem[pc++]; // длина поля
                x -= context->mem[pc++] + 1; // длина всей структуры
                for (i = 0; i < len; i++)
                    context->mem[x + i] = context->mem[x + i + di];
                x += len - 1;
                break;

            case SLICE:
                d = context->mem[pc++];
                i = context->mem[x--]; // index
                r = context->mem[x]; // array
                if (i < 0 || i >= context->mem[r - 1])
                    runtimeerr(context, index_out_of_range, i,
                               context->mem[r - 1]);
                context->mem[x] = r + i * d;
                break;
            case WIDEN:
                rf = (double)context->mem[x];
                memcpy(&context->mem[x++], &rf, sizeof(double));
                break;
            case WIDEN1:
                context->mem[x + 1] = context->mem[x];
                context->mem[x] = context->mem[x - 1];
                rf = (double)context->mem[x - 2];
                memcpy(&context->mem[x - 2], &rf, sizeof(double));
                ++x;
                break;
            case _DOUBLE:
                r = context->mem[x];
                context->mem[++x] = r;
                break;

            case ASS:
                context->mem[dsp(context, context->mem[pc++], l)] =
                    context->mem[x];
                break;
            case REMASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] %=
                    check_zero_int(context, context->mem[x]);
                context->mem[x] = r;
                break;
            case SHLASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] <<=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case SHRASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] >>=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case ANDASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] &=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case EXORASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] ^=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case ORASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] |=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case PLUSASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] +=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case MINUSASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] -=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case MULTASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] *=
                    context->mem[x];
                context->mem[x] = r;
                break;
            case DIVASS:
                r = context->mem[dsp(context, context->mem[pc++], l)] /=
                    check_zero_int(context, context->mem[x]);
                context->mem[x] = r;
                break;

            case ASSV:
                context->mem[dsp(context, context->mem[pc++], l)] =
                    context->mem[x--];
                break;
            case REMASSV:
                context->mem[dsp(context, context->mem[pc++], l)] %=
                    check_zero_int(context, context->mem[x--]);
                break;
            case SHLASSV:
                context->mem[dsp(context, context->mem[pc++], l)] <<=
                    context->mem[x--];
                break;
            case SHRASSV:
                context->mem[dsp(context, context->mem[pc++], l)] >>=
                    context->mem[x--];
                break;
            case ANDASSV:
                context->mem[dsp(context, context->mem[pc++], l)] &=
                    context->mem[x--];
                break;
            case EXORASSV:
                context->mem[dsp(context, context->mem[pc++], l)] ^=
                    context->mem[x--];
                break;
            case ORASSV:
                context->mem[dsp(context, context->mem[pc++], l)] |=
                    context->mem[x--];
                break;
            case PLUSASSV:
                context->mem[dsp(context, context->mem[pc++], l)] +=
                    context->mem[x--];
                break;
            case MINUSASSV:
                context->mem[dsp(context, context->mem[pc++], l)] -=
                    context->mem[x--];
                break;
            case MULTASSV:
                context->mem[dsp(context, context->mem[pc++], l)] *=
                    context->mem[x--];
                break;
            case DIVASSV:
                context->mem[dsp(context, context->mem[pc++], l)] /=
                    check_zero_int(context, context->mem[x--]);
                break;

            case ASSAT:
                r = context->mem[context->mem[x - 1]] = context->mem[x];
                context->mem[--x] = r;
                break;
            case REMASSAT:
                r = context->mem[context->mem[x - 1]] %=
                    check_zero_int(context, context->mem[x]);
                context->mem[--x] = r;
                break;
            case SHLASSAT:
                r = context->mem[context->mem[x - 1]] <<= context->mem[x];
                context->mem[--x] = r;
                break;
            case SHRASSAT:
                r = context->mem[context->mem[x - 1]] >>= context->mem[x];
                context->mem[--x] = r;
                break;
            case ANDASSAT:
                r = context->mem[context->mem[x - 1]] &= context->mem[x];
                context->mem[--x] = r;
                break;
            case EXORASSAT:
                r = context->mem[context->mem[x - 1]] ^= context->mem[x];
                context->mem[--x] = r;
                break;
            case ORASSAT:
                r = context->mem[context->mem[x - 1]] |= context->mem[x];
                context->mem[--x] = r;
                break;
            case PLUSASSAT:
                r = context->mem[context->mem[x - 1]] += context->mem[x];
                context->mem[--x] = r;
                break;
            case MINUSASSAT:
                r = context->mem[context->mem[x - 1]] -= context->mem[x];
                context->mem[--x] = r;
                break;
            case MULTASSAT:
                r = context->mem[context->mem[x - 1]] *= context->mem[x];
                context->mem[--x] = r;
                break;
            case DIVASSAT:
                r = context->mem[context->mem[x - 1]] /=
                    check_zero_int(context, context->mem[x]);
                context->mem[--x] = r;
                break;

            case ASSATV:
                context->mem[context->mem[x - 1]] = context->mem[x];
                x--;
                break;
            case REMASSATV:
                context->mem[context->mem[x - 1]] %=
                    check_zero_int(context, context->mem[x]);
                x--;
                break;
            case SHLASSATV:
                context->mem[context->mem[x - 1]] <<= context->mem[x];
                x--;
                break;
            case SHRASSATV:
                context->mem[context->mem[x - 1]] >>= context->mem[x];
                x--;
                break;
            case ANDASSATV:
                context->mem[context->mem[x - 1]] &= context->mem[x];
                x--;
                break;
            case EXORASSATV:
                context->mem[context->mem[x - 1]] ^= context->mem[x];
                x--;
                break;
            case ORASSATV:
                context->mem[context->mem[x - 1]] |= context->mem[x];
                x--;
                break;
            case PLUSASSATV:
                context->mem[context->mem[x - 1]] += context->mem[x];
                x--;
                break;
            case MINUSASSATV:
                context->mem[context->mem[x - 1]] -= context->mem[x];
                x--;
                break;
            case MULTASSATV:
                context->mem[context->mem[x - 1]] *= context->mem[x];
                x--;
                break;
            case DIVASSATV:
                context->mem[context->mem[x - 1]] /=
                    check_zero_int(context, context->mem[x]);
                x--;
                break;

            case LOGOR:
                context->mem[x - 1] = context->mem[x - 1] || context->mem[x];
                x--;
                break;
            case LOGAND:
                context->mem[x - 1] = context->mem[x - 1] && context->mem[x];
                x--;
                break;
            case LOR:
                context->mem[x - 1] |= context->mem[x];
                x--;
                break;
            case LEXOR:
                context->mem[x - 1] ^= context->mem[x];
                x--;
                break;
            case LAND:
                context->mem[x - 1] &= context->mem[x];
                x--;
                break;
            case LSHR:
                context->mem[x - 1] >>= context->mem[x];
                x--;
                break;
            case LSHL:
                context->mem[x - 1] <<= context->mem[x];
                x--;
                break;
            case LREM:
                context->mem[x - 1] %= context->mem[x];
                x--;
                break;
            case EQEQ:
                context->mem[x - 1] = context->mem[x - 1] == context->mem[x];
                x--;
                break;
            case NOTEQ:
                context->mem[x - 1] = context->mem[x - 1] != context->mem[x];
                x--;
                break;
            case LLT:
                context->mem[x - 1] = context->mem[x - 1] < context->mem[x];
                x--;
                break;
            case LGT:
                context->mem[x - 1] = context->mem[x - 1] > context->mem[x];
                x--;
                break;
            case LLE:
                context->mem[x - 1] = context->mem[x - 1] <= context->mem[x];
                x--;
                break;
            case LGE:
                context->mem[x - 1] = context->mem[x - 1] >= context->mem[x];
                x--;
                break;
            case LPLUS:
                context->mem[x - 1] += context->mem[x];
                x--;
                break;
            case LMINUS:
                context->mem[x - 1] -= context->mem[x];
                x--;
                break;
            case LMULT:
                context->mem[x - 1] *= context->mem[x];
                x--;
                break;
            case LDIV:
                context->mem[x - 1] /= check_zero_int(context, context->mem[x]);
                x--;
                break;
            case POSTINC:
                context->mem[++x] =
                    context->mem[r = dsp(context, context->mem[pc++], l)];
                context->mem[r]++;
                break;
            case POSTDEC:
                context->mem[++x] =
                    context->mem[r = dsp(context, context->mem[pc++], l)];
                context->mem[r]--;
                break;
            case INC:
                context->mem[++x] =
                    ++context->mem[dsp(context, context->mem[pc++], l)];
                break;
            case DEC:
                context->mem[++x] =
                    --context->mem[dsp(context, context->mem[pc++], l)];
                break;
            case POSTINCAT:
                context->mem[x] = context->mem[r = context->mem[x]];
                context->mem[r]++;
                break;
            case POSTDECAT:
                context->mem[x] = context->mem[r = context->mem[x]];
                context->mem[r]--;
                break;
            case INCAT:
                context->mem[x] = ++context->mem[context->mem[x]];
                break;
            case DECAT:
                context->mem[x] = --context->mem[context->mem[x]];
                break;
            case INCV:
            case POSTINCV:
                context->mem[dsp(context, context->mem[pc++], l)]++;
                break;
            case DECV:
            case POSTDECV:
                context->mem[dsp(context, context->mem[pc++], l)]--;
                break;
            case INCATV:
            case POSTINCATV:
                context->mem[context->mem[x--]]++;
                break;
            case DECATV:
            case POSTDECATV:
                context->mem[context->mem[x--]]--;
                break;

            case UNMINUS:
                context->mem[x] = -context->mem[x];
                break;

            case ASSR:
                context->mem[r = dsp(context, context->mem[pc++], l)] =
                    context->mem[x - 1];
                context->mem[r + 1] = context->mem[x];
                break;
            case PLUSASSR:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf += rf;
                memcpy(&context->mem[x - 1], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case MINUSASSR:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf -= rf;
                memcpy(&context->mem[x - 1], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case MULTASSR:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf *= rf;
                memcpy(&context->mem[x - 1], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case DIVASSR:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf /= check_zero_float(context, rf);
                memcpy(&context->mem[x - 1], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;

            case ASSATR:
                r = context->mem[x - 2];
                context->mem[r] = context->mem[x - 2] = context->mem[x - 1];
                context->mem[r + 1] = context->mem[x - 1] = context->mem[x];
                x--;
                break;
            case PLUSASSATR:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf += rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case MINUSASSATR:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf -= rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case MULTASSATR:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf *= rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;
            case DIVASSATR:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf /= check_zero_float(context, rf);
                memcpy(&context->mem[x++], &lf, sizeof(double));
                memcpy(&context->mem[i], &lf, sizeof(double));
                break;

            case ASSRV:
                r = dsp(context, context->mem[pc++], l);
                context->mem[r + 1] = context->mem[x--];
                context->mem[r] = context->mem[x--];
                memcpy(&lf, &context->mem[r], sizeof(double));
                break;
            case PLUSASSRV:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf += rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MINUSASSRV:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf -= rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MULTASSRV:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf *= rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case DIVASSRV:
                memcpy(&lf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                lf /= check_zero_float(context, rf);
                memcpy(&context->mem[i], &lf, sizeof(double));
                x -= 2;
                break;

            case ASSATRV:
                r = context->mem[x - 2];
                context->mem[r + 1] = context->mem[x--];
                context->mem[r] = context->mem[x--];
                break;
            case PLUSASSATRV:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf += rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                --x;
                break;
            case MINUSASSATRV:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf -= rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                --x;
                break;
            case MULTASSATRV:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf *= rf;
                memcpy(&context->mem[i], &lf, sizeof(double));
                --x;
                break;
            case DIVASSATRV:
                memcpy(&lf, &context->mem[i = context->mem[x -= 2]],
                       sizeof(double));
                lf /= check_zero_float(context, rf);
                memcpy(&context->mem[i], &lf, sizeof(double));
                --x;
                break;

            case EQEQR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf == rf;
                break;
            case NOTEQR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf != rf;
                break;
            case LLTR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf < rf;
                break;
            case LGTR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf > rf;
                break;
            case LLER:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf <= rf;
                break;
            case LGER:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                context->mem[x] = lf >= rf;
                break;
            case LPLUSR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                lf += rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                break;
            case LMINUSR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                lf -= rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                break;
            case LMULTR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                lf *= rf;
                memcpy(&context->mem[x++], &lf, sizeof(double));
                break;
            case LDIVR:
                memcpy(&lf, &context->mem[x -= 3], sizeof(double));
                lf /= check_zero_float(context, rf);
                memcpy(&context->mem[x++], &lf, sizeof(double));
                break;
            case POSTINCR:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                ++rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case POSTDECR:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                --rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case INCR:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                ++rf;
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case DECR:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                --rf;
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case POSTINCATR:
                memcpy(&rf, &context->mem[i = context->mem[x]], sizeof(double));
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                ++rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case POSTDECATR:
                memcpy(&rf, &context->mem[i = context->mem[x]], sizeof(double));
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                --rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case INCATR:
                memcpy(&rf, &context->mem[i = context->mem[x]], sizeof(double));
                ++rf;
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case DECATR:
                memcpy(&rf, &context->mem[i = context->mem[x]], sizeof(double));
                --rf;
                memcpy(&context->mem[x + 1], &rf, sizeof(double));
                x += 2;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case INCRV:
            case POSTINCRV:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                ++rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case DECRV:
            case POSTDECRV:
                memcpy(&rf,
                       &context->mem[i = dsp(context, context->mem[pc++], l)],
                       sizeof(double));
                --rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case INCATRV:
            case POSTINCATRV:
                memcpy(&rf, &context->mem[i = context->mem[x--]],
                       sizeof(double));
                ++rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;
            case DECATRV:
            case POSTDECATRV:
                memcpy(&rf, &context->mem[i = context->mem[x--]],
                       sizeof(double));
                --rf;
                memcpy(&context->mem[i], &rf, sizeof(double));
                break;

            case UNMINUSR:
                rf = -rf;
                memcpy(&context->mem[x - 1], &rf, sizeof(double));
                break;
            case LNOT:
                context->mem[x] = ~context->mem[x];
                break;
            case LOGNOT:
                context->mem[x] = !context->mem[x];
                break;

            default:
                runtimeerr(context, wrong_kop, context->mem[pc - 1], numTh);
        }
    }
    return NULL;
}

void
import(vm_context *context, const char *path)
{
    char  firstline[100];
    int   i, pc;
    FILE *input;

#ifdef ROBOT
    f1 = fopen(JD1, "r"); // файлы цифровых датчиков
    f2 = fopen(JD2, "r");
    printer_printf(&context->output_options, "stage 1\n");
    system("i2cset -y 2 0x48 0x10 0x1000 w"); // инициализация силовых моторов
    system("i2cset -y 2 0x48 0x11 0x1000 w");
    system("i2cset -y 2 0x48 0x12 0x1000 w");
    system("i2cset -y 2 0x48 0x13 0x1000 w");
#endif

    input = fopen(path, "r");
    if (input == NULL)
    {
        printer_printf(&context->error_options, "export.txt not found\n");
        return;
    }

    /* Check shebang, get back to start if there are none */
    if (fgets(firstline, sizeof(firstline), input) != firstline ||
        strncmp(firstline, "#!", 2) != 0)
    {
        fseek(input, 0, SEEK_SET);
    }

    fscanf(input, "%i %i %i %i %i %i %i\n", &pc, &context->funcnum,
           &context->id, &context->rp, &context->md, &context->maxdisplg,
           &context->wasmain);

    for (i = 0; i < pc; i++)
        fscanf(input, "%i ", &context->mem[i]);

    for (i = 0; i < context->funcnum; i++)
        fscanf(input, "%i ", &context->functions[i]);

    for (i = 0; i < context->id; i++)
        fscanf(input, "%i ", &context->identab[i]);

    for (i = 0; i < context->rp; i++)
        fscanf(input, "%i ", &context->reprtab[i]);

    for (i = 0; i < context->md; i++)
        fscanf(input, "%i ", &context->modetab[i]);

    fclose(input);

    context->threads[0] = pc;
    context->mem[pc] = context->g = pc + 2; // это l
    context->mem[context->g] = context->mem[context->g + 1] = 0;
    context->mem[pc + 1] = context->g + context->maxdisplg; // это x
    pc = 4;

    sem_unlink(sem_print);
    context->sempr = sem_open(sem_print, O_CREAT, S_IRUSR | S_IWUSR, 1);
    t_init(context);
    invoke_interpreter(context, &pc); // номер нити главной программы 0
    t_destroy(context);

#ifdef ROBOT
    system("i2cset -y 2 0x48 0x10 0 w"); // отключение силовых моторов
    system("i2cset -y 2 0x48 0x11 0 w");
    system("i2cset -y 2 0x48 0x12 0 w");
    system("i2cset -y 2 0x48 0x13 0 w");
    fclose(f1);
    fclose(f2);
#endif
}

static void
process_user_requests(vm_context *context, int argc, const char *argv[])
{
    int i;

    if (argc == 1)
        import(context, "export.txt");

    for (i = 1; i < argc; ++i)
    {
        import(context, argv[i]);
    }
}


int
main(int argc, const char *argv[])
{
    vm_context context;

    vm_context_init(&context);

    scanner_attach_file(&context.input_options, stdin);
    printer_attach_file(&context.output_options, stdout);
    printer_attach_file(&context.error_options, stderr);
    printer_attach_file(&context.miscout_options, stderr);
    process_user_requests(&context, argc, argv);

    vm_context_deinit(&context);
    return 0;
}
