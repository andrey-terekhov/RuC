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

#include "Defs.h"
extern int szof(int);

#define I2CBUFFERSIZE 50

#define index_out_of_range  1
#define wrong_kop           2
#define wrong_arr_init      3
#define wrong_motor_num     4
#define wrong_motor_pow     5
#define wrong_digsensor_num 6
#define wrong_ansensor_num  7
#define wrong_robot_com     8
#define wrong_number_of_elems 9
#define zero_devide         10
#define float_zero_devide   11
#define mem_overflow        12
#define sqrt_from_negat     13
#define log_from_negat      14
#define log10_from_negat    15
#define wrong_asin          16

int l, g, x, iniproc, maxdisplg, wasmain;
int reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id, modetab[MAXMODETAB], md;
int mem[MAXMEMSIZE], pc, functions[FUNCSIZE], funcnum;
int procd, iniprocs[INIPROSIZE], base = 0;
double lf, rf;
int N, bounds[100], d, i, from, prtype;
FILE *input;
int i,r, n, flagstop = 1, entry, num;

#ifdef ROBOT
FILE *f1, *f2;   // файлы цифровых датчиков
const char* JD1 = "/sys/devices/platform/da850_trik/sensor_d1";
const char* JD2 = "/sys/devices/platform/da850_trik/sensor_d2";

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

void printident(int r)
{
    r += 2;
    do
        printf_char(reprtab[r++]);
    while (reprtab[r] != 0);
}

int dspl(int d)
{
    return d < 0 ? g - d : l + d;
}

int dsp()
{
    return dspl(mem[pc++]);
}

void copy(int where, int from, int l)
{
    int i;
    for (i=0; i<l; i++)
        mem[where+i] =  mem[from+i];
}

void copyST(int from, int l)
{
    int i;
    for (i=0; i<l; i++)
        mem[++x] =  mem[from+i];
}

void copySTASS(int where, int l)
{
    int i, oldx = x -= l;
    for (i=0; i<l; i++)
        mem[where+i] = mem[++x];
    x = oldx;
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
            printf("попытка вычисления квадратного корня из отрицательного числа %f\n", rf);
            break;
        case log_from_negat:
            printf("попытка вычисления натурального логарифма из 0 или отрицательного числа%f\n", rf);
            break;
        case log10_from_negat:
            printf("попытка вычисления десятичного логарифма из 0 или отрицательного числа%f\n", rf);
            break;
        case wrong_asin:
            printf("аргумент арксинуса должен быть в отрезке [-1, 1], а тут %f\n", rf);
            break;
            
            
        default:
            break;
    }
    exit(3);
}

void prmem()
{
    int i;
    printf("mem=\n");
    for (i=g; i<=x; i++)
        printf("%i ) %i\n",i, mem[i]);
    printf("\n");
    
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
        printf("%f", rf);
    }
    else if (t == LVOID)
        printf(" значения типа ПУСТО печатать нельзя\n");
    
    // здесь t уже точно положительный
    else if (modetab[t] == MARRAY)
    {
        int rr = r, i, type = modetab[t+1], d;
        
        d = szof(type);
        
        if (modetab[t+1] > 0)
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
    int r;
//    printf("beg=%i t=%i\n", beg, t)
    if (t == LINT)
        scanf(" %i", &mem[beg]);
    else if (t == LCHAR)
    {
        mem[beg] = getf_char();
//        printf("\n");
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
        int rr = r, i, type = modetab[t+1], d;
        d = szof(type);
        rr = mem[beg];
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
        printf(" значения типа ФУНКЦИЯ и указателей печатать нельзя\n");

}

void interpreter();

void genarr(int N, int curdim, int d, int adr, int procnum, int oldpc)
{
    int c0, i, curb = bounds[curdim];
//    mem[++x] = curdim < N ? 1 : d;
    mem[++x] = curb;
    c0 = ++x;

    x += curb * (curdim < N ? 1 : d) - 1;
    
    if (x >= MAXMEMSIZE)
        runtimeerr(mem_overflow, 0, 0);
    
    if(curdim == N)
    {
        if (procnum)
        {
            int curx = x, oldbase = base;
            for (i=c0; i<=curx; i+=d)
            {
                pc = procnum;   // вычисление границ очередного массива в структуре
                base = i;
                interpreter();
            }
            pc = oldpc;
            base = oldbase;
        }
    }
    else
    {
        for (i=0; i < curb; i++)
        {
            genarr(N, curdim + 1, d, c0 + i, procnum, oldpc);
        }
    }
    mem[adr] = c0;
}


void rec_init_arr(int where, int N, int d)
{
    int b = mem[where-1], i, j;
//    double f;
//      printf("rec_init_arr where= %i from= %i b= %i N= %i d= %i\n", where, from, b, N, d);
    for (i=0; i<b; i++)
        if (N == 1)
        {
            for (j=0; j<d; j++)
                mem[where++] = mem[from++];
//            memcpy(&f, &mem[from-2], sizeof(double));
//            printf("f= %f\n", f);
        }
        else
            rec_init_arr(mem[where++], N-1, d);
}

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

void interpreter()
{
    flagstop = 1;
    while (flagstop)
    {
        memcpy(&rf, &mem[x-1], sizeof(double));
        //        printf("pc=%i mem[pc]=%i rf=%f\n", pc, mem[pc], rf);
        
        switch (mem[pc++])
        {
            case STOP:
                flagstop = 0;
                break;
    #ifdef ROBOT
            case SETMOTOR:
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
            case CGETDIGSENSOR:
                n = mem[x];
                if (n < 1 || n > 2)
                    runtimeerr(wrong_digsensor_num, n, 0);
                if (n == 1)
                    fscanf(f1, "%i", &i);
                else
                    fscanf(f2, "%i", &i);
                mem[x] = i;
                break;
            case CGETANSENSOR:
                n = mem[x];
                if (n < 1 || n > 6)
                    runtimeerr(wrong_ansensor_num, n, 0);
                memset(i2ccommand, '\0', I2CBUFFERSIZE);
                printf("i2cget -y 2 0x48 0x%x\n", 0x26 - n);
                snprintf(i2ccommand, I2CBUFFERSIZE, "i2cget -y 2 0x48 0x%x", 0x26 - n);
                mem[x] = rungetcommand(i2ccommand);
                break;
            case SLEEP:
                sleep(mem[x--]);
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
                printident(identab[i+1]);   // ссылка на reprtab
                
                if (prtype > 0 && modetab[prtype] == MARRAY && modetab[prtype+1] > 0)
                    auxprint(dspl(identab[i+3]), identab[i+2], '\n', '\n');
                else
                    auxprint(dspl(identab[i+3]), identab[i+2], ' ', '\n');
                
                break;
            case GETID:
                i = mem[pc++];              // ссылка на identtab
                prtype = identab[i+2];
                printident(identab[i+1]);   // ссылка на reprtab
                printf(" ");
                auxget(dspl(identab[i+3]), identab[i+2]);
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
                mem[x] = (int)(rf+0.5);
                break;
                
            case STRUCTWITHARR:
            {
                int oldpc, oldbase = base, procnum;
                base = dsp();
                procnum = mem[pc++];
                oldpc = pc;
                pc = procnum;
                interpreter();
                pc = oldpc;
                base = oldbase;
                flagstop = 1;
            }
                break;
            case DEFARR:      // N, d, displ, proc     на стеке N1, N2, ... , NN
            {
                int N = mem[pc++];
                int d = mem[pc++];
                int curdsp = mem[pc++];
                int proc = mem[pc++];
                for (i=abs(N); i>0; i--)
                    if ((bounds[i] = mem[x--]) <= 0)
                        runtimeerr(wrong_number_of_elems, 0, bounds[i]);
                
                genarr(abs(N), 1, d, N > 0 ? dspl(curdsp) : base + curdsp, proc, pc);
                flagstop = 1;
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
                mem[++x] = mem[dsp()];
                break;
            case LOADD:
                memcpy(&mem[++x], &mem[dsp()], sizeof(double));
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
                mem[++x] = dsp();
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
                if (x >= MAXMEMSIZE)
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
                copy(dspl(mem[pc]), dspl(mem[pc+1]), mem[pc+2]);
                pc += 3;
                break;
            case COPY01:
                copy(dspl(mem[pc]), mem[x--], mem[pc+1]);
                pc +=2;
                break;
            case COPY10:
                copy(mem[x--], dspl(mem[pc]), mem[pc+1]);
                pc += 2;
                break;
            case COPY11:
                r = mem[x--];
                copy(mem[x--], r, mem[pc++]);
                break;
            case COPY0ST:
                copyST(dspl(mem[pc]), mem[pc+1]);
                pc += 2;
                break;
            case COPY1ST:
                r = mem[x--];
                copyST(r, mem[pc++]);
                break;
            case COPY0STASS:
                copySTASS(dspl(mem[pc]), mem[pc+1]);
                pc += 2;
                break;
            case COPY1STASS:
                r = mem[x-1];
                copySTASS(r, mem[pc++]);
                x--;
                break;
                
            case SLICE:
                d = mem[pc++];
                i = mem[x--];     // index
                r = mem[x];       // array
                if (i < 0 || i >= mem[r-1])
                    runtimeerr(index_out_of_range, i, mem[r-1]);
                mem[x] = r + i * d;
                break;
            case ARRINIT:
                N = mem[pc++];       // N - размерность
                d = mem[pc++];       // d
                x -= mem[pc++] * d;  // сколько всего значений
                from = x + 1;
//                memcpy(&lf, &mem[from], sizeof(double));
//                printf("lf= %f\n", lf);
                rec_init_arr(mem[dsp()], N, d);
                break;
            case STRUCTINIT:
                d=mem[pc++];     // сколько всего значений во всех полях
                x -= d;
                from = x + 1;
                r = dsp();       // адрес структуры в статике
                for (i=0; i<d; i++)
                    mem[r++] = mem[from++];
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
                mem[dsp()] = mem[x];
                break;
            case REMASS:
                r = mem[dsp()] %= check_zero_int(mem[x]);
                mem[x] = r;
                break;
            case SHLASS:
                r = mem[dsp()] <<= mem[x];
                mem[x] = r;
                break;
            case SHRASS:
                r = mem[dsp()] >>= mem[x];
                mem[x] = r;
                break;
            case ANDASS:
                r = mem[dsp()] &= mem[x];
                mem[x] = r;
                break;
            case EXORASS:
                r = mem[dsp()] ^= mem[x];
                mem[x] = r;
                break;
            case ORASS:
                r = mem[dsp()] |= mem[x];
                mem[x] = r;
                break;
            case PLUSASS:
                r = mem[dsp()] += mem[x];
                mem[x] = r;
                break;
            case MINUSASS:
                r = mem[dsp()] -= mem[x];
                mem[x] = r;
                break;
            case MULTASS:
                r = mem[dsp()] *= mem[x];
                mem[x] = r;
                break;
            case DIVASS:
                r = mem[dsp()] /= check_zero_int(mem[x]);
                mem[x] = r;
                break;
                
            case ASSV:
                mem[dsp()] = mem[x--];
                break;
            case REMASSV:
                mem[dsp()] %= check_zero_int(mem[x--]);
                break;
            case SHLASSV:
                mem[dsp()] <<= mem[x--];
                break;
            case SHRASSV:
                mem[dsp()] >>= mem[x--];
                break;
            case ANDASSV:
                mem[dsp()] &= mem[x--];
                break;
            case EXORASSV:
                mem[dsp()] ^= mem[x--];
                break;
            case ORASSV:
                mem[dsp()] |= mem[x--];
                break;
            case PLUSASSV:
                mem[dsp()] += mem[x--];
                break;
            case MINUSASSV:
                mem[dsp()] -= mem[x--];
                break;
            case MULTASSV:
                mem[dsp()] *= mem[x--];
                break;
            case DIVASSV:
                mem[dsp()] /= check_zero_int(mem[x--]);
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
                mem[++x] = mem[r=dsp()];
                mem[r]++;
                break;
            case POSTDEC:
                mem[++x] = mem[r=dsp()];
                mem[r]--;
                break;
            case INC:
                mem[++x] = ++mem[dsp()];
                break;
            case DEC:
                mem[++x] = --mem[dsp()];
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
                mem[dsp()]++;
                break;
            case DECV:
            case POSTDECV:
                mem[dsp()]--;
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
                mem[r=dsp()] = mem[x-1];
                mem[r+1] = mem[x];
                break;
            case PLUSASSR:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf += rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MINUSASSR:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf -= rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MULTASSR:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf *= rf;
                memcpy(&mem[x-1], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case DIVASSR:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
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
                memcpy(&mem[++x], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MINUSASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf -= rf;
                memcpy(&mem[++x], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case MULTASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf *= rf;
                memcpy(&mem[++x], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
            case DIVASSATR:
                memcpy(&lf, &mem[i=mem[x-=2]], sizeof(double));
                lf /= check_zero_float(rf);
                memcpy(&mem[++x], &lf, sizeof(double));
                memcpy(&mem[i], &lf, sizeof(double));
                break;
 
            case ASSRV:
                r = dsp();
                mem[r+1] = mem[x--];
                mem[r] = mem[x--];
                break;
            case PLUSASSRV:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf += rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MINUSASSRV:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf -= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case MULTASSRV:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
                lf *= rf;
                memcpy(&mem[i], &lf, sizeof(double));
                x -= 2;
                break;
            case DIVASSRV:
                memcpy(&lf, &mem[i=dsp()], sizeof(double));
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
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case POSTDECR:
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                --rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case INCR:
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
                ++rf;
                memcpy(&mem[x+1], &rf, sizeof(double));
                x += 2;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECR:
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
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
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
                ++rf;
                memcpy(&mem[i], &rf, sizeof(double));
                break;
            case DECRV:
            case POSTDECRV:
                memcpy(&rf, &mem[i=dsp()], sizeof(double));
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
                memcpy(&mem[x-1], &rf, sizeof(num));
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
}

void import()
{
    
#ifdef ROBOT
    f1 = fopen(JD1, "r");                       // файлы цифровых датчиков
    f2 = fopen(JD2, "r");
    printf("stage 1\n");
    system("i2cset -y 2 0x48 0x10 0x1000 w");   // инициализация силовых моторов
    system("i2cset -y 2 0x48 0x11 0x1000 w");
    system("i2cset -y 2 0x48 0x12 0x1000 w");
    system("i2cset -y 2 0x48 0x13 0x1000 w");
#endif
    
    input = fopen("../../../export.txt", "r");
    
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
    x = g + maxdisplg;
    pc = 0;
    
    interpreter();
    
#ifdef ROBOT
    printf("111");
    system("i2cset -y 2 0x48 0x10 0 w");   // отключение силовых моторов
    system("i2cset -y 2 0x48 0x11 0 w");
    system("i2cset -y 2 0x48 0x12 0 w");
    system("i2cset -y 2 0x48 0x13 0 w");
    fclose(f1);
    fclose(f2);
#endif
    
    
}
