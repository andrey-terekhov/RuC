//  RuC2A
//
//  Created by Andrey Terekhov on 24/Apr/16.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
// http://www.lysator.liu.se/c/ANSI-C-grammar-y.html
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include "import.h"

#include "global_vars.h"
#include "Defs.h"

/* Названия файлов ввода/вывода */
#define GLOBAL_KEYWORDS_FILENAME    "keywords.txt"
#define GLOBAL_TREE_FILENAME        "tree.txt"
#define GLOBAL_CODES_FILENAME       "codes.txt"
#define GLOBAL_EXPORT_FILENAME      "export.txt"

const char * name = "../../../tests/roboterr.c";
             /*"../../../tests/Golovan/dining_philosophers.c";*/

int toreprtab(char str[])
{
    int i, oldrepr = rp;
    hash = 0;
    rp += 2;
    for (i=0; str[i] != 0; i++)
    {
        hash += str[i];
        reprtab[rp++] = str[i];
    }
    hash &= 255;
    reprtab[rp++] = 0;
    reprtab[oldrepr] = hashtab[hash] ;
    reprtab[oldrepr+1] = 1;
    return hashtab[hash] = oldrepr;
}

/* Первичная инициализация глобальных объектов */
void init()
{
    memset(hashtab, 0, sizeof(hashtab));
}

/* Чтение ключевых слов в reprtab */
void read_keywords(const char *path)
{
    keywordsnum = 1;

    input = fopen(path, "r");
    if (input == NULL)
    {
        fprintf(stderr, "файл ключевых слов не найден: %s\n", path);
        exit(1);
    }
    getnext();
    nextch();
    while (scan() != LEOF)   // чтение ключевых слов
        ;
    fclose(input);
    input = NULL;
}

/* Инициализация modetab */
void
init_modetab()
{
    // занесение в modetab описателя struct {int numTh; int inf; }
    modetab[1] = 0;
    modetab[2] = MSTRUCT;
    modetab[3] = 2;
    modetab[4] = 4;
    modetab[5] = modetab[7] = LINT;
    modetab[6] = toreprtab("numTh");
    modetab[8] = toreprtab("data");

    // занесение в modetab описателя функции void t_msg_send(struct msg_info m)
    modetab[9] = 1;
    modetab[10] = MFUNCTION;
    modetab[11] = LVOID;
    modetab[12] = 1;
    modetab[13] = 2;

    // занесение в modetab описателя функции void* interpreter(void* n)
    modetab[14] = 9;
    modetab[15] = MFUNCTION;
    modetab[16] = LVOIDASTER;
    modetab[17] = 1;
    modetab[18] = LVOIDASTER;
    modetab[19] = startmode = 14;
    md = 19;
    keywordsnum = 0;
    lines[line = 1] = 1;
    charnum = 1;
    kw = 1;
    tc = 0;
}

int main(int argc, const char * argv[])
{
    int i;

    init();

    read_keywords(GLOBAL_KEYWORDS_FILENAME);

#if 0
input  = fopen(name, "r");        //   исходный текст
    output = fopen("macro.txt", "wt");

    if (input == NULL)
    {
        printf(" не найден файл %s\n", name);
        exit(1);
    }
#endif

#if 0
    getnext();
    nextch();
    next = scan();

    preprocess_file();                //   макрогенерация

    fclose(output);
    fclose(input);

    return 0;

    input  = fopen("macro.txt", "r");
#endif

    init_modetab();

    if (argc < 2) {
        input = fopen(name, "r");
    } else {
        input = fopen(argv[1], "r");
    }

    if (input == NULL)
    {
        fprintf(stderr, "файл не найден: %s\n", name);
        exit(1);
    }

    /* Генерация дерева */
    output = fopen(GLOBAL_TREE_FILENAME, "wt");

    getnext();
    nextch();
    next = scan();

    ext_decl();

    lines[line+1] = charnum;
    tablesandtree();
    fclose(output);

    /* Генерация кода */
    output = fopen(GLOBAL_CODES_FILENAME, "wt");

    codegen();
    tablesandcode();

    fclose(input);
    fclose(output);

    /* Вывод таблиц в файл */
    output = fopen(GLOBAL_EXPORT_FILENAME, "wt");
    fprintf(output, "%i %i %i %i %i %i %i\n",
            pc, funcnum, id, rp, md, maxdisplg, wasmain);

    for (i=0; i<pc; i++)
        fprintf(output, "%i ", mem[i]);
    fprintf(output, "\n");

    for (i=0; i<funcnum; i++)
        fprintf(output, "%i ", functions[i]);
    fprintf(output, "\n");

    for (i=0; i<id; i++)
        fprintf(output, "%i ", identab[i]);
    fprintf(output, "\n");

    for (i=0; i<rp; i++)
        fprintf(output, "%i ", reprtab[i]);

    for (i=0; i<md; i++)
        fprintf(output, "%i ", modetab[i]);
    fprintf(output, "\n");

    fclose(output);

    /* Запуск интерпретатора на экспортированных таблицах */
    if (notrobot && (argc < 2))
        import(GLOBAL_EXPORT_FILENAME);

    return 0;
}

