//
//  import.c
//
//  Created by Andrey Terekhov on 2/25/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

//#define ROBOT 1
//#include <unistd.h>
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "th_static.h"


// Я исхожу из того, что нумерация нитей процедурой t_create начинается с 1 и идет последовательно
// в соответствии с порядком вызовов этой процудуры, главная программа имеет номер 0. Если стандарт POSIX
// этого не обеспечивает, это должен сделать Саша Головань.

// Память mem, как обычно, начинается с кода всей программы, включая нити, затем идут глобальные данные,
// затем куски для стеков и массивов гланой программы и нитей, каждый кусок имеет размер MAXMEMTHREAD.

// Поскольку и при запуске главной прграммы, и при запуске любой нити процудура t_create получает в качестве
// параметра одну и ту же процедуру interpreter, важно, чтобы при начале работы программы или нити были
// правильно установлены l, x и pc, причем все важные переменные были локальными, тогда дальше все
// переключения между нитями будут заботой ОС.

// Есть глобальный массив threads, i-ый элемент которого указывает на начало куска i-ой нити.
// Каждый кусок начинается с шапки, где хранятся l, x и pc, которые нужно установить в момент старта нити.

#include "Defs.h"
extern int szof(int);

#define I2CBUFFERSIZE 50

#define index_out_of_range    1
#define wrong_kop             2
#define wrong_arr_init        3
#define wrong_motor_num       4
#define wrong_motor_pow       5
#define wrong_digsensor_num   6
#define wrong_ansensor_num    7
#define wrong_robot_com       8
#define wrong_number_of_elems 9
#define zero_devide          10
#define float_zero_devide    11
#define mem_overflow         12
#define sqrt_from_negat      13
#define log_from_negat       14
#define log10_from_negat     15
#define wrong_asin           16
#define wrong_string_init    17
#define printf_runtime_crash 18

int g, iniproc, maxdisplg, wasmain;
int reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id, modetab[MAXMODETAB], md;
int mem[MAXMEMSIZE], functions[FUNCSIZE], funcnum;
int threads[NUMOFTHREADS]; //, curthread, upcurthread;
int procd, iniprocs[INIPROSIZE], base = 0;
FILE *input;

#ifdef ROBOT
FILE *f1, *f2;   // файлы цифровых датчиков
const char* JD1 = "/sys/devices/platform/da850_trik/sensor_d1";
const char* JD2 = "/sys/devices/platform/da850_trik/sensor_d2";

int szof(int type)
{
    return type == LFLOAT ? 2 :
    (type > 0 && modetab[type] == MSTRUCT) ? modetab[type + 1] : 1;
}

int rungetcommand(const char *command)
{
    FILE *fp;
    int x = -1;
    char path[100] = {'\0'};
    
    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL)
        runtimeerr(wrong_robot_com, 0,0);
    
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL)
    {
        x = strtol(path, NULL, 16);
        printf("[%s] %d\n", path, x);
    }
    pclose(fp);
    return x;                   // ??????
}

#endif

void printf_char(int wchar)
{
    if (wchar<128)
        printf("%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

        printf("%c%c", first, second);
    }
}

void fprintf_char(FILE *f, int wchar)
{    if (wchar<128)
    fprintf(f, "%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

        fprintf(f, "%c%c", first, second);
    }
}

int getf_char()
{
    // reads UTF-8
    
    unsigned char firstchar, secondchar;
    
    if (scanf(" %c", &firstchar) == EOF)
        return EOF;
    else
        if ((firstchar & /*0b11100000*/0xE0) == /*0b11000000*/0xC0)
        {
            scanf("%c", &secondchar);
            return ((int)(firstchar & /*0b11111*/0x1F)) << 6 | (secondchar & /*0b111111*/0x3F);
        }
        else
            return firstchar;
}

void runtimeerr(int e, int i, int r)
{
    switch (e)
    {
        case index_out_of_range:
            printf("индекс %i за пределами границ массива %i\n", i, r-1);
            break;
        case wrong_kop:
            printf("команду %i я пока не реализовал\n", i);
            break;
        case wrong_arr_init:
            printf("массив с %i элементами инициализируется %i значениями\n", i, r);
            break;
        case wrong_string_init:
            printf("строковая переменная с %i элементами инициализируется строкой с %i литерами\n", i, r);
            break;
        case wrong_motor_num:
            printf("номер силового мотора %i, а должен быть от 1 до 4\n", i);
            break;
        case wrong_motor_pow:
            printf("задаваемая мощность мотора %i равна %i, а должна быть от -100 до 100\n", i, r);
            break;
        case wrong_digsensor_num:
            printf("номер цифрового сенсора %i, а должен быть 1 или 2\n", i);
            break;
        case wrong_ansensor_num:
            printf("номер аналогового сенсора %i, а должен быть от 1 до 6\n", i);
            break;
        case wrong_robot_com:
            printf("робот не может исполнить команду\n");
            break;
        case wrong_number_of_elems:
            printf("количество элементов в массиве по каждому измерению должно быть положительным, а тут %i\n", r);
            break;
        case zero_devide:
            printf("целое деление на 0\n");
            break;
        case float_zero_devide:
            printf("вещественное деление на 0\n");
            break;
        case mem_overflow:
            printf("переполнение памяти, скорее всего, нет выхода из рекурсии\n");
            break;
        case sqrt_from_negat:
            printf("попытка вычисления квадратного корня из отрицательного числа \n");
            break;
        case log_from_negat:
            printf("попытка вычисления натурального логарифма из 0 или отрицательного числа\n");
            break;
        case log10_from_negat:
            printf("попытка вычисления десятичного логарифма из 0 или отрицательного числа\n");
            break;
        case wrong_asin:
            printf("аргумент арксинуса должен быть в отрезке [-1, 1]\n");
            break;
            
        case printf_runtime_crash:
            printf("странно, printf не работает на этапе исполнения; ошибка коммпилятора");
            break;
        default:
            break;
    }
    exit(3);
}
/*
void prmem()
{
    int i;
    printf("mem=\n");
    for (i=g; i<=x; i++)
        printf("%i ) %i\n",i, mem[i]);
    printf("\n");
    
}
*/

void auxprintf(int strbeg, int databeg)
{
    int i, curdata = databeg + 1;
    for (i = strbeg; mem[i] != 0; ++i)
    {
        if (mem[i] == '%')
        {
            switch (mem[++i])
            {
                case 'i':
                case 1094:  // ц
                    printf("%i", mem[curdata++]);
                    break;

                case 'c':
                case 1083:  // л
                    printf_char(mem[curdata++]);
                    break;

                case 'f':
                case 1074:  // в
                    printf("%lf", *((double*) (&mem[curdata])));
                    curdata += 2;
                    break;

                case '%':
                    printf("%%");
                    break;

                default:

                    break;
            }
        }
        else
            printf_char(mem[i]);
    }
}

void auxprint(int beg, int t, char before, char after)
{
    double rf;
    int r = mem[beg];
    
    if (before)
        printf("%c", before);
    
    if (t == LINT)
        printf("%i", r);
    else if (t == LCHAR)
        printf_char(r);
    else if (t == LFLOAT)
    {
        memcpy(&rf, &mem[beg], sizeof(double));
        printf("%lf", rf);
    }
    else if (t == LVOID)
        printf(" значения типа ПУСТО печатать нельзя\n");
    
    // здесь t уже точно положительный
    else if (modetab[t] == MARRAY)
    {
        int rr = r, i, type = modetab[t+1], d;
        d = szof(type);
        
        if (type > 0)
            for (i=0; i<mem[rr-1]; i++)
                auxprint(rr + i * d, type, 0, '\n');
        else
            for (i=0; i<mem[rr-1]; i++)
                auxprint(rr + i * d, type, 0, (type == LCHAR ? 0 : ' '));
    }
    else if (modetab[t] == MSTRUCT)
    {
        int cnt = modetab[t+2], i;
        printf("{");
        for (i=2; i<=cnt; i+=2)
        {
            int type = modetab[t+i+1];
            if (type < 0)
                auxprint(beg, type, (i == 2 ? 0 : ' '), (i == cnt ? 0 : ','));
            else
                auxprint(beg, type, '\n', '\n');
            beg += szof(type);
        }
        printf("}");
    }
    else
        printf(" значения типа ФУНКЦИЯ и указателей печатать нельзя\n");
    
    if (after)
        printf("%c", after);
}

void auxget(int beg, int t)
{
    double rf;
//     printf("beg=%i t=%i\n", beg, t);
    if (t == LINT)
        scanf(" %i", &mem[beg]);
    else if (t == LCHAR)
    {
        mem[beg] = getf_char();
    }
    else if (t == LFLOAT)
    {
        scanf(" %lf", &rf);
        memcpy(&mem[beg], &rf, sizeof(double));
    }
    else if (t == LVOID)
        printf(" значения типа ПУСТО вводить нельзя\n");
    
    // здесь t уже точно положительный
    else if (modetab[t] == MARRAY)
    {
        int rr = mem[beg], i, type = modetab[t+1], d;
        d = szof(type);
        for (i=0; i<mem[rr-1]; i++)
            auxget(rr + i * d, type);
    }
    else if (modetab[t] == MSTRUCT)
    {
        int cnt = modetab[t+2], i;
        for (i=2; i<=cnt; i+=2)
        {
            int type = modetab[t+i+1];
            auxget(beg, type);
            beg += szof(type);
        }
    }
    else
        printf(" значения типа ФУНКЦИЯ и указателей вводить нельзя\n");
}

void* interpreter(void*);

int check_zero_int(int r)
{
    if (r == 0)
        runtimeerr(zero_devide, 0, 0);
    return r;
    
}

double check_zero_float(double r)
{
    if (r == 0)
        runtimeerr(float_zero_devide, 0, 0);
    return r;
}

int dsp(int di, int l)
{
    return di < 0 ? g - di : l + di;
}

void* interpreter(void* pcPnt)
{
    int l, x, pc = *((int*) pcPnt), numTh;// = t_getThNum();
    int N, bounds[100], d,from, prtype, cur0;
    int i,r, flagstop = 1, entry, di, di1, len;
    double lf, rf;
    threads[numTh] = cur0 = (numTh == 0 ? threads[0] : threads[numTh-1] + MAXMEMTHREAD);
    // область нити начинается с номера нити, потом идут 3 служебных слова процедуры
    mem[cur0] = numTh;
    l = mem[cur0+1];
    x = mem[cur0+2]+3;
    
//    printf("interpreter session numTh=%i l=%i x=%i pc=%i\n", numTh, l, x, pc);
    
    flagstop = 1;
    while (flagstop)
    {
        memcpy(&rf, &mem[x-1], sizeof(double));
            //    printf("pc=%i mem[pc]=%i rf=%f\n", pc, mem[pc], rf);
        
        //printf("running th #%i, cmd=%i\n", t_getThNum(), mem[pc]);
        switch (mem[pc++])
        {

            case STOP:
                flagstop = 0;
                threads[numTh+2] = x;
                break;
/*
            case CREATEDIRECTC:
               t_create(interpreter, (void*)&pc);
                flagstop = 1;
                break;
            case CREATEC:
                i = mem[x];
                entry = functions[i > 0 ? i : mem[l-i]];
                pc = entry + 3;
                t_create(interpreter, (void*)&pc);
                flagstop = 1;
                break;
 
            case JOINC:
                t_join(mem[x--]);
                break;
 
            case SLEEPC:
                t_sleep(mem[x--]);
                break;
                
            case EXITDIRECTC:
            case EXITC:
                printf("found exitc thread = %i\n", t_getThNum());
                t_exit();
                break;
 
            case SEMCREATEC:
                mem[x] = t_sem_create(mem[x]);
                break;
 
            case SEMPOSTC:
                t_sem_post(mem[x--]);
                break;
 
            case SEMWAITC:
                t_sem_wait(mem[x--]);
                break;
            
            case INITC:
                t_init();
                break;
                
            case DESTROYC:
                t_destroy();
                break;
                
            case MSGRECEIVEC:
            {
                struct msg_info m = t_msg_receive();
                mem[++x] = m.numTh;
                mem[++x] = m.data;
            }
                break;
 
            case MSGSENDC:
            {
                struct msg_info m;
                m.data = mem[x--];
                m.numTh = mem[x--];
                t_msg_send(m);
            }
                break;*/

    #ifdef ROBOT

            case SETMOTORC:
                r = mem[x--];
                n = mem[x--];
                if (n < 1 || n > 4)
                    runtimeerr(wrong_motor_num, n, 0);
                if (r < -100 || r > 100)
                    runtimeerr(wrong_motor_pow, n, r);
                memset(i2ccommand, '\0', I2CBUFFERSIZE);
                printf("i2cset -y 2 0x48 0x%x 0x%x b\n", 0x14 + n - 1, r);
                snprintf(i2ccommand, I2CBUFFERSIZE, "i2cset -y 2 0x48 0x%x 0x%x b", 0x14 + n - 1, r);
                system(i2ccommand);
                break;
                
            case GETDIGSENSORC:
                n = mem[x];
                if (n < 1 || n > 2)
                    runtimeerr(wrong_digsensor_num, n, 0);
                if (n == 1)
                    fscanf(f1, "%i", &i);
                else
                    fscanf(f2, "%i", &i);
                mem[x] = i;
                break;
                
            case GETANSENSORC:
                n = mem[x];
                if (n < 1 || n > 6)
                    runtimeerr(wrong_ansensor_num, n, 0);
                memset(i2ccommand, '\0', I2CBUFFERSIZE);
                printf("i2cget -y 2 0x48 0x%x\n", 0x26 - n);
                snprintf(i2ccommand, I2CBUFFERSIZE, "i2cget -y 2 0x48 0x%x", 0x26 - n);
                mem[x] = rungetcommand(i2ccommand);
                break;
    #endif
            case FUNCBEG:
                pc = mem[pc+1];
                break;
            case PRINT:
            {
                int t = mem[pc++];
                x -= szof(t);
                auxprint(x+1, t, 0, '\n');
            }
                break;
            case PRINTID:
                i = mem[pc++];              // ссылка на identtab
                prtype = identab[i+2];
                r = identab[i+1] + 2;       // ссылка на reprtab
                do
                    printf_char(reprtab[r++]);
                while (reprtab[r] != 0);
                
                if (prtype > 0 && modetab[prtype] == MARRAY && modetab[prtype+1] > 0)
                    auxprint(dsp(identab[i+3], l), prtype, '\n', '\n');
                else
                    auxprint(dsp(identab[i+3], l), prtype, ' ', '\n');
                break;

            /* Ожидает указатель на форматную строку на верхушке стека
             * Принимает единственным параметром суммарный размер того, что нужно напечатать
             * Проверок на типы не делает, этим занимался компилятор
             * Если захотим передавать динамически формируемые строки, нужно будет откуда-то брать весь набор типов печатаемого
             */
            case PRINTF:
            {
                int sumsize = mem[pc++];
                int strbeg = mem[x--];

                auxprintf(strbeg, x -= sumsize);
            }
                break;
            case GETID:
                i = mem[pc++];              // ссылка на identtab
                prtype = identab[i+2];
                r = identab[i+1] + 2;       // ссылка на reprtab
                do
                    printf_char(reprtab[r++]);
                while (reprtab[r] != 0);
                printf("\n");
                auxget(dsp(identab[i+3], l), prtype);
                break;
            case ABSIC:
                mem[x] = abs(mem[x]);
                break;
            case ABSC:
                rf = fabs(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case SQRTC:
                if (rf < 0)
                    runtimeerr(sqrt_from_negat, 0, 0);
                rf = sqrt(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case EXPC:
                rf = exp(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case SINC:
                rf = sin(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case COSC:
                rf = cos(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case LOGC:
                if (rf <= 0)
                    runtimeerr(log_from_negat, 0, 0);
                rf = log(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case LOG10C:
                if (rf <= 0)
                    runtimeerr(log10_from_negat, 0, 0);
                rf = log10(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case ASINC:
                if (rf < -1 || rf > 1)
                    runtimeerr(wrong_asin, 0, 0);
                rf = asin(rf);
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case RANDC:
                rf = (double)rand() / RAND_MAX;
                memcpy(&mem[++x], &rf, sizeof(double));
                ++x;
                break;
            case ROUNDC:
                mem[--x] = rf < 0 ? (int)(rf-0.5) : (int)(rf+0.5);
                break;
                
            case STRUCTWITHARR:
            {
                int oldpc, oldbase = base, procnum;
                base = (di = mem[pc++], di<0) ? g-di : l+di;
                procnum = mem[pc++];
                oldpc = pc;
                pc = procnum;
//                mem[threads[numTh]+1] = l;
                mem[threads[numTh]+2] = x;
                interpreter((void*) &pc);
                x = threads[numTh+2];
                pc = oldpc;
                base = oldbase;
                flagstop = 1;
           }
                break;
            case DEFARR:      // N, d, displ, proc     на стеке N1, N2, ... , NN
            {
                int N =      mem[pc++];
                int d =      mem[pc++];
                int curdsp = mem[pc++];
                int proc =   mem[pc++];
                int stackC0[10], stacki[10], i, curdim = 1;
                
                for (i=abs(N); i>0; i--)
                    if ((bounds[i] = mem[x--]) <= 0)
                        runtimeerr(wrong_number_of_elems, 0, bounds[i]);
                stacki[1] = 0;
                
                mem[++x] = bounds[1];
                mem[N > 0 ? (curdsp < 0 ? g - curdsp : l + curdsp) : base + curdsp] = stackC0[1] = x + 1;
                N = abs(N);
                x += bounds[1] * (curdim < N ? 1 : d);
                
                if (x >= threads[numTh] + MAXMEMTHREAD)
                    runtimeerr(mem_overflow, 0, 0);
                if (N == 1)
                {
                    if (proc)
                    {
                        int curx = x, oldbase = base, oldpc = pc, i;
                        for (i=stackC0[1]; i<=curx; i+=d)
                        {
                            pc = proc;   // вычисление границ очередного массива в структуре
                            base = i;
                            mem[threads[numTh]+2] = x;
                            interpreter((void*) &pc);
                            flagstop = 1;
                            x = threads[numTh+2];
                        }
                        pc = oldpc;
                        base = oldbase;
                    }
                }
                else
                {
              lab1: do
                    {
                // go down
                        mem[++x] = bounds[curdim+1];
                        mem[stackC0[curdim] + stacki[curdim]++] = stackC0[curdim+1] = x + 1;
                        x += bounds[curdim+1] * (curdim == N-1 ? d : 1);
                        
                        if (x >= threads[numTh] + MAXMEMTHREAD)
                            runtimeerr(mem_overflow, 0, 0);
                        ++curdim;
                        stacki[curdim] = 0;
                    }
                    while (curdim < N);
                // построена очередная вертикаль подмассивов
                   
                    if (proc)
                    {
                        int curx = x, oldbase = base, oldpc = pc, i;
                        for (i=stackC0[curdim]; i<=curx; i+=d)
                        {
                            pc = proc;   // вычисление границ очередного массива в структуре
                            base = i;
                            mem[threads[numTh]+2] = x;
                            interpreter((void*) &pc);
                            flagstop = 1;
                            x = threads[numTh+2];
                        }
                        pc = oldpc;
                        base = oldbase;
                    }
                // go right
                    --curdim;
                    if (stacki[curdim] < bounds[curdim])
                        goto lab1;
                // go up
                    if (curdim-- != N-1)
                        goto lab1;
                }
            }
                break;
            case LI:
                mem[++x] = mem[pc++];
                break;
            case LID:
                memcpy(&mem[++x], &mem[pc++], sizeof(double));
                ++x;
                ++pc;
                break;
            case LOAD:
                mem[++x] = mem[dsp(mem[pc++], l)];
                break;
            case LOADD:
                memcpy(&mem[++x], &mem[dsp(mem[pc++], l)], sizeof(double));
                ++x;
                break;
            case LAT:
                mem[x] = mem[mem[x]];
                break;
            case LATD:
                r = mem[x];
                mem[x++] = mem[r];
                mem[x] = mem[r+1];
                break;
            case LA:
                mem[++x] = dsp(mem[pc++], l);
                break;
            case CALL1:
                mem[l+1] = ++x;
                mem[x++] = l;
                mem[x++] = 0;    // следующая статика
                mem[x] = 0;      // pc в момент вызова
                break;
            case CALL2:
                i = mem[pc++];
                entry = functions[i > 0 ? i : mem[l-i]];
                l = mem[l+1];
                x = l + mem[entry+1] - 1;
                if (x >= threads[numTh] + MAXMEMTHREAD)
                    runtimeerr(mem_overflow, 0, 0);
                mem[l+2] = pc;
                pc = entry + 3;
                break;
            case RETURNVAL:
                d = mem[pc++];
                pc = mem[l+2];
                r = l;
                l = mem[l];
                mem[l+1] = 0;
                from = x-d;
                x = r-1;
                for (i=0; i<d; i++)
                    mem[++x] = mem[++from];
                break;
            case RETURNVOID:
                pc = mem[l+2];
                x = l-1;
                l = mem[l];
                mem[l+1] = 0;
                break;
            case NOP:
                ;
                break;
            case B:
            case STRING:
                pc = mem[pc];
                break;
            case BE0:
                pc = (mem[x--]) ?  pc + 1 : mem[pc];
                break;
            case BNE0:
                pc = (mem[x--]) ? mem[pc] : pc + 1;
                break;
            case SELECT:
                mem[x] += mem[pc++];   // ident displ
                break;
            case COPY00:
                di = dsp(mem[pc++], l);
                di1 = dsp(mem[pc++], l);
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY01:
                di = dsp(mem[pc++], l);
                len = mem[pc++];
                di1 = mem[x--];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY10:
                di =  mem[x];
                di1 = dsp(mem[pc++], l);
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY10V:
                di =  mem[x--];
                di1 = dsp(mem[pc++], l);
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY11:
                di1 = mem[x--];
                di =  mem[x];
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY11V:
                di1 = mem[x--];
                di =  mem[x--];
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[di+i] =  mem[di1+i];
                break;
            case COPY0ST:
                di = dsp(mem[pc++], l);
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[++x] =  mem[di+i];

                break;
            case COPY1ST:
                di = mem[x--];
                len = mem[pc++];
                for (i=0; i<len; i++)
                    mem[++x] =  mem[di+i];
                break;
            case COPY0STASS:
                di = dsp(mem[pc++], l);
                len = mem[pc++];
                x -= len;
                for (i=0; i<len; i++)
                    mem[di+i] = mem[x+i+1];
                break;
            case COPY1STASS:
                len = mem[pc++];
                x -= len;
                di = mem[x];
                for (i=0; i<len; i++)
                    mem[di+i] = mem[x+i+1];
                break;
            case COPY1STASSV:
                len = mem[pc++];
                x -= len;
                di = mem[x--];
                for (i=0; i<len; i++)
                    mem[di+i] = mem[x+i+2];
                break;
                
            case SLICE:
                d = mem[pc++];
                i = mem[x--];        // index
                r = mem[x];          // array
                if (i < 0 || i >= mem[r-1])
                    runtimeerr(index_out_of_range, i, mem[r-1]);
                mem[x] = r + i * d;
                break;
            case STRINGINIT:
                di = mem[pc++];
                r = mem[di < 0 ? g - di : l + di];
                N = mem[r-1];
                from = mem[x--];
                d = mem[from-1];     // d - кол-во литер в строке-инициаторе
                if (N != d)
                    runtimeerr(wrong_string_init, N, d);
                for (i=0; i<N; i++)
                    mem[r+i] = mem[from+i];
                break;
            case ARRINIT:
                N = mem[pc++];       // N - размерность
                d = mem[pc++];       // d
                x -= mem[pc++];      // сколько всего слов во всех элементах инициализации
                from = x + 1;
            {
                int stA[10], stN[10], sti[10], stpnt = 1, i, j;
                stA[1] = mem[dsp(mem[pc++], l)];
                stN[1] = mem[stA[1]-1];
                for (i=2; i<10; ++i)
                    sti[i] = 0;
                sti[1] = -1;
                if (N == 1)
                {
                    for (i=0; i<stN[1]; i+=d)
                        for (j=0; j<d; j++)
                            mem[stA[stpnt]+i+j] = mem[from++];
                }
                else
                {
            goright:
                    if (stpnt == 1 && sti[1] == stN[1]-1)
                        goto labexit;
                    else /* if (++sti[stpnt] < stN[stpnt]) */
                    {
                        if (++sti[stpnt] == stN[stpnt])
                        {
                            --stpnt;
                            goto goright;
                        }
                        stA[stpnt] += sti[stpnt];
                    }
            godown: while (stpnt < N)
                    {
                        stA[stpnt+1] = mem[stA[stpnt]];
                        sti[++stpnt] = 0;
                        stN[stpnt] = mem[stA[stpnt]-1];

                    }
                for (i=0; i<stN[stpnt]; i+=d)
                        for (j=0; j<d; j++)
                        {
                            mem[stA[stpnt]+i+j] = mem[from++];
                        }
                if (N > 1)
                    --stpnt;
                goto goright;
                }
            labexit:;
            }
                break;
            case WIDEN:
                rf = (double)mem[x];
                memcpy(&mem[x++], &rf, sizeof(double));
                break;
            case WIDEN1:
                mem[x+1] = mem[x];
                mem[x] = mem[x-1];
                rf = (double)mem[x-2];
                memcpy(&mem[x-2], &rf, sizeof(double));
                ++x;
                break;
            case _DOUBLE:
                r = mem[x];
                mem[++x] = r;
                break;
                
                
            case ASS:
                mem[dsp(mem[pc++], l)] = mem[x];
                break;
            case REMASS:
                r = mem[dsp(mem[pc++], l)] %= check_zero_int(mem[x]);
                mem[x] = r;
                break;
            case SHLASS:
                r = mem[dsp(mem[pc++], l)] <<= mem[x];
                mem[x] = r;
                break;
            case SHRASS:
                r = mem[dsp(mem[pc++], l)] >>= mem[x];
                mem[x] = r;
                break;
            case ANDASS:
                r = mem[dsp(mem[pc++], l)] &= mem[x];
                mem[x] = r;
                break;
            case EXORASS:
                r = mem[dsp(mem[pc++], l)] ^= mem[x];
                mem[x] = r;
                break;
            case ORASS:
                r = mem[dsp(mem[pc++], l)] |= mem[x];
                mem[x] = r;
                break;
            case PLUSASS:
                r = mem[dsp(mem[pc++], l)] += mem[x];
                mem[x] = r;
                break;
            case MINUSASS:
                r = mem[dsp(mem[pc++], l)] -= mem[x];
                mem[x] = r;
                break;
            case MULTASS:
                r = mem[dsp(mem[pc++], l)] *= mem[x];
                mem[x] = r;
                break;
            case DIVASS:
                r = mem[dsp(mem[pc++], l)] /= check_zero_int(mem[x]);
                mem[x] = r;
                break;
                
            case ASSV:
                mem[dsp(mem[pc++], l)] = mem[x--];
                break;
            case REMASSV:
                mem[dsp(mem[pc++], l)] %= check_zero_int(mem[x--]);
                break;
            case SHLASSV:
                mem[dsp(mem[pc++], l)] <<= mem[x--];
                break;
            case SHRASSV:
                mem[dsp(mem[pc++], l)] >>= mem[x--];
                break;
            case ANDASSV:
                mem[dsp(mem[pc++], l)] &= mem[x--];
                break;
            case EXORASSV:
                mem[dsp(mem[pc++], l)] ^= mem[x--];
                break;
            case ORASSV:
                mem[dsp(mem[pc++], l)] |= mem[x--];
                break;
            case PLUSASSV:
                mem[dsp(mem[pc++], l)] += mem[x--];
                break;
            case MINUSASSV:
                mem[dsp(mem[pc++], l)] -= mem[x--];
                break;
            case MULTASSV:
                mem[dsp(mem[pc++], l)] *= mem[x--];
                break;
            case DIVASSV:
                mem[dsp(mem[pc++], l)] /= check_zero_int(mem[x--]);
                break;
                
            case ASSAT:
                r = mem[mem[x-1]] = mem[x];
                mem[--x] = r;
                break;
            case REMASSAT:
                r = mem[mem[x-1]] %= check_zero_int(mem[x]);
                mem[--x] = r;
                break;
            case SHLASSAT:
                r = mem[mem[x-1]] <<= mem[x];
                mem[--x] = r;
                break;
            case SHRASSAT:
                r = mem[mem[x-1]] >>= mem[x];
                mem[--x] = r;
                break;
            case ANDASSAT:
                r = mem[mem[x-1]] &= mem[x];
                mem[--x] = r;
                break;
            case EXORASSAT:
                r = mem[mem[x-1]] ^= mem[x];
                mem[--x] = r;
                break;
            case ORASSAT:
                r = mem[mem[x-1]] |= mem[x];
                mem[--x] = r;
                break;
            case PLUSASSAT:
                r = mem[mem[x-1]] += mem[x];
                mem[--x] = r;
                break;
            case MINUSASSAT:
                r = mem[mem[x-1]] -= mem[x];
                mem[--x] = r;
                break;
            case MULTASSAT:
                r = mem[mem[x-1]] *= mem[x];
                mem[--x] = r;
                break;
            case DIVASSAT:
                r = mem[mem[x-1]] /= check_zero_int(mem[x]);
                mem[--x] = r;
                break;
                
            case ASSATV:
                mem[mem[x-1]] = mem[x];
                x--;
                break;
            case REMASSATV:
                mem[mem[x-1]] %= check_zero_int(mem[x]);
                x--;
                break;
            case SHLASSATV:
                mem[mem[x-1]] <<= mem[x];
                x--;
                break;
            case SHRASSATV:
                mem[mem[x-1]] >>= mem[x];
                x--;
                break;
            case ANDASSATV:
                mem[mem[x-1]] &= mem[x];
                x--;
                break;
            case EXORASSATV:
                mem[mem[x-1]] ^= mem[x];
                x--;
                break;
            case ORASSATV:
                mem[mem[x-1]] |= mem[x];
                x--;
                break;
            case PLUSASSATV:
                mem[mem[x-1]] += mem[x];
                x--;
                break;
            case MINUSASSATV:
                mem[mem[x-1]] -= mem[x];
                x--;
                break;
            case MULTASSATV:
                mem[mem[x-1]] *= mem[x];
                x--;
                break;
            case DIVASSATV:
                mem[mem[x-1]] /= check_zero_int(mem[x]);
                x--;
                break;
                
            case LOGOR:
                mem[x-1] = mem[x-1] || mem[x];
                x--;
                break;
            case LOGAND:
                mem[x-1] = mem[x-1] && mem[x];
                x--;
                break;
            case LOR:
                mem[x-1] |= mem[x];
                x--;
                break;
            case LEXOR:
                mem[x-1] ^= mem[x];
                x--;
                break;
            case LAND:
                mem[x-1] &= mem[x];
                x--;
                break;
            case LSHR:
                mem[x-1] >>= mem[x];
                x--;
                break;
            case LSHL:
                mem[x-1] <<= mem[x];
                x--;
                break;
            case LREM:
                mem[x-1] %= mem[x];
                x--;
                break;
            case EQEQ:
                mem[x-1] = mem[x-1] == mem[x];
                x--;
                break;
            case NOTEQ:
                mem[x-1] = mem[x-1] != mem[x];
                x--;
                break;
            case LLT:
                mem[x-1] = mem[x-1] < mem[x];
                x--;
                break;
            case LGT:
                mem[x-1] = mem[x-1] > mem[x];
                x--;
                break;
            case LLE:
                mem[x-1] = mem[x-1] <= mem[x];
                x--;
                break;
            case LGE:
                mem[x-1] = mem[x-1] >= mem[x];
                x--;
                break;
            case LPLUS:
                mem[x-1] += mem[x];
                x--;
                break;
            case LMINUS:
                mem[x-1] -= mem[x];
                x--;
                break;
            case LMULT:
                mem[x-1] *= mem[x];
                x--;
                break;
            case LDIV:
                mem[x-1] /= check_zero_int(mem[x]);
                x--;
                break;
            case POSTINC:
                mem[++x] = mem[r=dsp(mem[pc++], l)];
                mem[r]++;
                break;
            case POSTDEC:
                mem[++x] = mem[r=dsp(mem[pc++], l)];
                mem[r]--;
                break;
            case INC:
                mem[++x] = ++mem[dsp(mem[pc++], l)];
                break;
            case DEC:
                mem[++x] = --mem[dsp(mem[pc++], l)];
                break;
            case POSTINCAT:
                mem[x] = mem[r=mem[x]];
                mem[r]++;
                break;
            case POSTDECAT:
                mem[x] = mem[r=mem[x]];
                mem[r]--;
                break;
            case INCAT:
                mem[x] = ++mem[mem[x]];
                break;
            case DECAT:
                mem[x] = --mem[mem[x]];
                break;
            case INCV:
            case POSTINCV:
                mem[dsp(mem[pc++], l)]++;
                break;
            case DECV:
            case POSTDECV:
                mem[dsp(mem[pc++], l)]--;
                break;
            case INCATV:
            case POSTINCATV:
                mem[mem[x--]]++;
                break;
            case DECATV:
            case POSTDECATV:
                mem[mem[x--]]--;
                break;
                
            case UNMINUS:
                mem[x] = -mem[x];
                break;
 
            case ASSR:
                mem[r=dsp(mem[pc++], l)] = mem[x-1];
                mem[r+1] = mem[x];
                break;
            case PLUSASSR:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf += rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MINUSASSR:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf -= rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MULTASSR:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf *= rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case DIVASSR:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
                
            case ASSATR:
                r = mem[x-2];
                mem[r] = mem[x-2] = mem[x-1];
                mem[r+1] = mem[x-1] = mem[x];
                x--;
                break;
            case PLUSASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf += rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MINUSASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf -= rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MULTASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf *= rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case DIVASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[x++], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
 
            case ASSRV:
                r = dsp(mem[pc++], l);
                mem[r+1] = mem[x--];
                mem[r] = mem[x--];
                memcpy(&lf, &mem[r], sizeof(double));
                break;
            case PLUSASSRV:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf += rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MINUSASSRV:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf -= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MULTASSRV:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf *= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case DIVASSRV:
                memcpy(&lf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            
            case ASSATRV:
                r = mem[x-2];
                mem[r+1] = mem[x--];
                mem[r] = mem[x--];
                break;
            case PLUSASSATRV:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf += rf;
                memcpy(&mem[i], &lf, sizeof(double));
                --x;
                break;
            case MINUSASSATRV:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf -= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                --x;
                break;
            case MULTASSATRV:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf *= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                --x;
                break;
            case DIVASSATRV:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[i], &lf, sizeof(double));
                --x;
                break;
                
            case EQEQR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf == rf;
                break;
            case NOTEQR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf != rf;
                break;
            case LLTR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf < rf;
                break;
            case LGTR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf > rf;
                break;
            case LLER:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf <= rf;
                break;
            case LGER:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                mem[x] = lf >= rf;
                break;
            case LPLUSR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                lf += rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                break;
            case LMINUSR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                lf -= rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                break;
            case LMULTR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                lf *= rf;
                memcpy(&mem[x++], &lf, sizeof(double));
                break;
            case LDIVR:
                memcpy(&lf, &mem[x-=3], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[x++], &lf, sizeof(double));
                break;
            case POSTINCR:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case POSTDECR:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                --rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case INCR:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                ++rf;
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECR:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                --rf;
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case POSTINCATR:
                memcpy(&rf, &mem[i=mem[x]], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x+=2;
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case POSTDECATR:
                memcpy(&rf, &mem[i=mem[x]], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x+=2;
                --rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case INCATR:
                memcpy(&rf, &mem[i=mem[x]], sizeof(double));
                ++rf;
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECATR:
                memcpy(&rf, &mem[i=mem[x]], sizeof(double));
                --rf;
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case INCRV:
            case POSTINCRV:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECRV:
            case POSTDECRV:
                memcpy(&rf, &mem[i=dsp(mem[pc++], l)], sizeof(double));
                --rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case INCATRV:
            case POSTINCATRV:
                memcpy(&rf, &mem[i=mem[x--]], sizeof(double));
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECATRV:
            case POSTDECATRV:
                memcpy(&rf, &mem[i=mem[x--]], sizeof(double));
                --rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
                
            case UNMINUSR:
                rf = -rf;
                memcpy(&mem[x-1], &rf, sizeof(double));
                break;
            case LNOT:
                mem[x] = ~mem[x];
                break;
            case LOGNOT:
                mem[x] = !mem[x];
                break;
                
            default:
                runtimeerr(wrong_kop, mem[pc-1], 0);
        }
    }
    
    return NULL;
}

void import()
{
    int i, l, pc, x;
    
#ifdef ROBOT
    f1 = fopen(JD1, "r");                       // файлы цифровых датчиков
    f2 = fopen(JD2, "r");
    printf("stage 1\n");
    system("i2cset -y 2 0x48 0x10 0x1000 w");   // инициализация силовых моторов
    system("i2cset -y 2 0x48 0x11 0x1000 w");
    system("i2cset -y 2 0x48 0x12 0x1000 w");
    system("i2cset -y 2 0x48 0x13 0x1000 w");
#endif
    
    input = fopen("export.txt", "r");
    
    fscanf(input, "%i %i %i %i %i %i %i\n", &pc, &funcnum, &id, &rp, &md, &maxdisplg, &wasmain);

    for (i=0; i<pc; i++)
        fscanf(input, "%i ", &mem[i]);
    
    for (i=0; i<funcnum; i++)
        fscanf(input, "%i ", &functions[i]);
    
    for (i=0; i<id; i++)
        fscanf(input, "%i ", &identab[i]);
    
    for (i=0; i<rp; i++)
        fscanf(input, "%i ", &reprtab[i]);
    
    for (i=0; i<md; i++)
        fscanf(input, "%i ", &modetab[i]);
    
    fclose(input);
    
    l = g = pc;
    mem[g] = mem[g+1] = 0;
    threads[0] = x = g + maxdisplg;
    mem[x+1] = l;
    mem[x+2] = x;
    pc = 4;
    //t_init();
    interpreter(&pc);                      // номер нити главной программы 0
    //t_destroy();
 
#ifdef ROBOT
    system("i2cset -y 2 0x48 0x10 0 w");   // отключение силовых моторов
    system("i2cset -y 2 0x48 0x11 0 w");
    system("i2cset -y 2 0x48 0x12 0 w");
    system("i2cset -y 2 0x48 0x13 0 w");
    fclose(f1);
    fclose(f2);
#endif
    
}
