#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "global_vars.h"


int macrotext [MAXREPRTAB];
char mstring[100];
int macrofunction [MAXREPRTAB];
int functionident [MAXREPRTAB];
char fchange [50]; 
int fip = 1;
int mfp = 1;
int mfirstrp = -1; // начало и конец макрослов в reprtab
int mlastrp = -1;
int mp = 3;
int msp = 0;
int ifln = 0;
int mcl;

extern void endofline();
extern void endnl();
extern int getnext();
extern void onemore();
extern int letter();
extern int digit();
extern int equal();
//extern void printf_char(int wchar);

int mletter(int r);
int mdigit(int r);
int mequal(int str[], int j);

void to_macrotext(char chang[], int oldrepr);
void macro_reprtab(char str[],char chang []);
void from_macrotext(char str[]);
int macro_keywords();
void relis_define();

void toreprtab_f(char str[]);
void to_functionident ();
void from_functionident(int r);
void create_chenge(int r1);
void r_macrofunction();

void r_coment();
void m_ident();
int faind_ident(char str[]);
void end_line();

int check_if(int type_if);
void folse_if ();
int m_folse();
void m_true(int type_if);
void m_if(int type_if);

void macroscan();
void preprocess_file();

void printf_char1(int wchar)
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

void show_macro()
{
    int i1 = lines[line-1];
    int i2 = mlines[m_conect_lines[line-1]];
    char str1[50];
    char str2[50];
    int j = 0;
    int k;
    int flag=1;
    int i = 0;
    while(i1 != charnum)
    {
        if(source[i1] == before_source[i2])
        {
            str1[j++]=before_source[i2];
            i1++;
            i2++;
        }
        else
        {
            flag=0;

            while((before_source[i2] >= 'A' && before_source[i2] <= 'Z') || (before_source[i2] >='a' && before_source[i2] <= 'z') 
             || before_source[i2] == '_' || (before_source[i2] >= 0x410/*А */ && before_source[i2] <= 0x44F /*'я'*/) ||
              before_source[i2] >='0' && before_source[i2] <= '9')
            {
                i = 0;
                str2[i++] = before_source[i2];
                str1[j++] = before_source[i2];
                i2++;
            }
            from_macrotext(str2);

            i1 += msp;
        }
    }

    printf("Line %i) ", m_conect_lines[line]);

    for (k = 1; k < j;k++)
    {
    printf_char1(str1[k]);
    }
    if(flag == 0)
    {
      printf("В строке есть макрозамена, строка после макрогенерации:\nline) ");
        for ( k = lines[line]; k < charnum;k++)
        {
        printf_char1(source[k]);
        }                  
    }
}

int mletter(int r)
{
    return (macrofunction[r] >= 'A' && macrofunction[r] <= 'Z') || (macrofunction[r] >='a' && macrofunction[r] <= 'z') 
    || macrofunction[r] == '_' || (macrofunction[r] >= 0x410/*А */ && macrofunction[r] <= 0x44F /*'я'*/);
}

int mdigit(int r)
{
    return macrofunction[r] >='0' && macrofunction[r] <= '9';
}

int mequal(int str[], int j)
{
    int i = 0 ;
    //printf("fi = '%c'",functionident[j]);    
    while (str[i++] == functionident[j++])
    {
        //printf("fi = '%c'",functionident[j]);
        if (str[i] == 0 && functionident[j] == 0)
            return 1;
    }
    return 0;
}


void m_nextch()
{
    int j;
    
    onemore();
    if (curchar == EOF)
    {
        mlines[++mline] = m_charnum;
        mlines[mline+1] = m_charnum;
        return;
    }  

    before_source[m_charnum++] = curchar;
        
    if (curchar == '\n')
    {
        mlines[++mline] = m_charnum;
        mlines[mline+1] = m_charnum;
        if (kw)
        {
            printf("Line %i) ", mline-1);
            for ( j=mlines[mline-1]; j<mlines[mline]; j++)
                if (before_source[j] != EOF)
                    printf_char1(before_source[j]);
        }
    }
    return;
}

void m_fprintf(int a)
{
    if(a == '\n')
    {
    m_conect_lines[mcl++] = mline-1;
   // printf("mline%i) ", mcl)
    }
   // printf("%c", a);
    fprintf(output, "%c", a);

    return;
}

void to_macrotext(char chang[], int oldrepr)
{
    int i;
   // printf ("to_macrotext\n");
    macrotext[mp++] = oldrepr;
    for( i = 0; chang[i] != 0; i++)
    {
        macrotext[mp++] = chang[i];
    }
    macrotext[mp++] = 0;
}

void macro_reprtab(char str[],char chang [])
{
    //printf ("macro_reprtab str = %s; chang = %s; ",str, chang);
    int oldrepr = rp;
    int r,i;


    mlastrp = oldrepr;
    hash = 0;
    rp += 2;

    for( i=0; str[i] != 0; i++)
    {
        hash += str[i];
        reprtab[rp++] = str[i];
    }
    hash &= 255;
    reprtab[rp++] = 0;
    reprtab[oldrepr] = hashtab[hash] ;
    reprtab[oldrepr+1] = mp;

    r = hashtab[hash];
    while(r != 0)
    {
        if(equal(r, oldrepr))
        {
            //error(stping_befor_use_for_cange);
        }
        else
        {
            r = reprtab[r];
        }
    }
    to_macrotext(chang, oldrepr);
    hashtab[hash] = oldrepr;
    //printf ("hash = %d; hashtab = %d; \n\n", hash, hashtab[hash]);
}

void from_macrotext(char str[])
{ 
    int i;
    //printf("from_macrotext, %s\n", str);
    msp = 0;
    int r = faind_ident(str);
    //printf(" r = %d\n", r);

    if(r)
    {
        
        //printf ("Замена \n");
        if (reprtab[r + 1] == 2)
        {
           // printf ("Функция \n");
            from_functionident(r);
            return;
        }

        r = reprtab[r + 1] + 1;

        for( ; macrotext[r] != 0; r++)
        {
            mstring[msp++] = macrotext[r];
            //m_fprintf(macrotext[r]); 
        }

        return;
    }
    
    for( i = 0; str[i] != 0; i++)
    {
        mstring[msp++] = str[i];
        //m_fprintf(str[i]);
    }
    //printf("нет замены\n");
}

int macro_keywords() 
{
    //printf("macro_keywords\n\n");
    int oldrepr = rp;
    int r = 0;

    rp+=2;
    hash = 0;
                
    do
    {                 
    hash += curchar;
    reprtab[rp++] = curchar;
    m_nextch();
    //printf ("c %c; ",curchar);
    }
    while(letter() || digit());

    if (curchar != ' ' && curchar != '\n' && curchar != '\t')
    {
        printf("Неправильное ключевое слово, далее должен идти символ ' 'или'\\n'или'\\t' \n");
        exit (10);
    }
    else
    {
      m_nextch();
    }
                
    hash &= 255;
    reprtab[rp++] = 0;
    r = hashtab[hash];
    if(r)
    {
      do
        {
         //printf ("r %d; ",r);
         //return 0;

        if(equal(r, oldrepr))
        {
            rp = oldrepr;
            return(reprtab[r+1] < 0) ? reprtab[r+1] : (repr = r, IDENT);// ?
        }
        else
        r = reprtab[r];
        }
        while(r);
    }
    return 0;
}

void relis_define()
{
   //printf(" define\n");

    if (letter())
    { 
        //printf("Ident\n");
        int i = 0;
        char str[30] = {"\0"};
        while(letter() || digit())
        {
            str[i++] = curchar;
            m_nextch();
        }

        if (curchar == '(')
        { 

            toreprtab_f(str);
            //printf("замена на функцию\n");
            m_nextch();
            r_macrofunction ();
            return;
        }
        else if(curchar != ' ')
        {
            m_nextch();
            printf("плохой символ1\n %c %i\n", curchar, curchar);
            exit(10);
        }
        else
        {
            //printf("макрозамена\n");
            m_nextch();
            i=0;
            char chang [30] = {"\0"};
            while(curchar != '\n')
            {
                chang[i++] = curchar;
                m_nextch();
            }
            macro_reprtab(str, chang);
            return;
        }
    }
    else
    {
        printf("плохой символ2 %c %i\n", curchar, curchar);
        exit(10); 
    }
}

void toreprtab_f(char str[])
{
    int i ;
    int oldrepr = rp;
    mlastrp = oldrepr;
    hash = 0;
    rp += 2;
    for (i=0; str[i] != 0; i++)
    {
        hash += str[i];
        reprtab[rp++] = str[i];
    }
    hash &= 255;
    reprtab[rp++] = 0;
    reprtab[rp++] = fip;
    reprtab[rp++] = 0;
    reprtab[oldrepr] = hashtab[hash] ;
    reprtab[oldrepr+1] = 2;
    hashtab[hash] = oldrepr;
}

void to_functionident ()
{
    while(curchar != ')')
    {
        fip++;
        if (letter())
        {
            while(letter() || digit())
            {
                functionident[fip++] = curchar;
               // printf("'%c'",curchar);
                m_nextch();
            }
            functionident[fip++] = 0;    
        }
        else 
        {
            printf("Фунция неправильно написана1");
            exit(10);
        }

        if(curchar == ',' && nextchar == ' ')
        {
            m_nextch();
            m_nextch(); 
           // printf("',' ' '");  
        }
        else if (curchar != ')')
        {
            printf("Фунция неправильно написана2");
            exit(10);
        }
    }

   // printf("\n");
    m_nextch();
    return; 
}

void from_functionident(int r)
{
    int i,kp,cp;
    int r1 = r + 2;
    int str[30];

    for(; reprtab[r1] !=  0; r1++);
    r1++;
    r1 = reprtab[r1];
    create_chenge(r1);
    //printf("fcange\n\n");

    int finish = functionident[r1];
    int newfi = functionident[r1 + 1];
    int flag = 1;
    msp = 0;
    while(macrofunction[newfi] != '\n' ) 
    {
        //printf(" !%c! ",macrofunction[newfi]);
        if(mletter(newfi))
        {
            flag = 1;
            for ( i = 0; i < 30; i++)
            {
                str[i] = 0;
            }
            i = 0;
            while(mletter(newfi) || mdigit(newfi))
            {
                //printf(" &%c& ",macrofunction[newfi]);
                str[i++] = macrofunction[newfi++];
            }
            //printf(" str = %s ",str);
            for( kp = r1 + 2; kp < finish; )
            {
               // printf(" equale = %d ",mequal(str, kp + 1));
                if (mequal(str, kp + 1))
                {
                    for( cp = functionident[kp]; fchange[cp] != '\n'; cp++)
                    {
                        mstring[msp++] = fchange[cp];
                        //m_fprintf(fchange[cp]); 
                        // printf(" mВывод '%c' ",fchange[cp]);
                    }
                    flag = 0;
                    break;   
                }
                while(functionident[kp++] != 0);
            }
            if (flag == 1)
            {
                for( i = 0; str[i] != 0; i++)
                {
                   mstring[msp++] = str[i]; 
                }
            }
        }
        else
        {
       mstring[msp++] = macrofunction[newfi];
      //  printf("Вывод '%c' ",macrofunction[newfi]);
        newfi++;
        }
    }
   

}

void create_chenge(int r1)
{
    int i;
    int r = r1 + 2;
    int cp = 1;
    functionident[r] = cp;
    //printf("1 fi[r1 = %d] ='%d' ",r1, functionident[r1]);
    if(curchar == '(')
    { 
        m_nextch();
        while(curchar != ')')
        {
            if (curchar == ',')
            {
                m_nextch();
                m_nextch();
                for(; functionident[r] != 0; r++);
                if(r < functionident[r1])
                {
                    fchange[cp++] = '\n';
                    r++;
                    functionident[r] =cp;
                    //printf(" fi[%d] ='%c' ",r, functionident[r]);
                }
                else
                {
                    printf("У этой функции меньше параметров");
                    exit(10);
                }
            }
            else
            {
                if(letter())
                {
                    m_ident();
                    for(i = 0; i < msp; i++)
                    {
                        fchange[cp++] =  mstring[i];
                    }
                }
                else
                {
                fchange[cp++] = curchar;
                //printf("'%c'",curchar);
                m_nextch();
                }
            }
        }
        m_nextch();
        fchange[cp++] = '\n';
    } 
    else
    {
        printf("Неверное использование функций");
        exit(10);
    }
}

void r_macrofunction()
{
    int j;
    int olderfip = fip++;
    functionident[fip++] = mfp;

   // printf("'%c'\n",curchar);
    to_functionident ();
    m_nextch();
    functionident[olderfip] = fip;

    /*printf("functionident r0=%d  r1='%d'\n",olderfip ,functionident[olderfip]);
    for(int i = olderfip; i<fip; i++)
    {
        printf(" i=%d c='%c' d='%d'\n",i ,functionident[i], functionident[i]);
    }*/

    while(curchar != '\n')
    {
    
        if (letter())
        {
            m_ident();
            for ( j = 0; j < msp; j++)
            {
                macrofunction[mfp++] =  mstring[j];  
            }
            msp = 0;
        }
        else
        {
        macrofunction [mfp++] = curchar;
        m_nextch();
        }

    }
    macrofunction [mfp++] = '\n';
    return;
}

void r_coment()
{

    m_fprintf(curchar);
    m_nextch(); 
    if (curchar == '/')
    {
        m_fprintf(curchar);

        while (curchar != '\n')
        {
            m_nextch();
            m_fprintf(curchar);
            if (curchar == EOF)
            {
            return;
            }
        }
        return;
    }
    
    else if (curchar == '*')
    {
        do
        {
            m_fprintf(curchar);
            m_nextch();
            if (curchar == EOF)
            {
                return;
            }   
        }
        while(curchar != '*' || nextchar != '/');
        m_fprintf(curchar);
        m_nextch();
        m_fprintf(curchar);
        m_nextch();

        return;
    }
  
}

void m_ident()
{
    int i = 0;
    char str[30] = {"\0"};
    while(letter() || digit())
    {
        str[i++] = curchar;
        m_nextch();
        // printf ("Mc str = %s\n",str);
    }
    from_macrotext(str);

    return;
}

int faind_ident(char str[])
{
    int fpr = rp;
    int i, r;
    hash = 0;
    fpr += 2;
    for( i = 0; str[i] != 0; i++)
    {
        hash += str[i];
        reprtab[fpr++] = str[i];
    }
    reprtab[fpr++] = 0;
    hash &= 255;
    r = hashtab[hash];
    while(r)
    {
        //printf ("mfirstrp = %d; r = %d; mlastrp = %d equal = %d \n", mfirstrp, r, mlastrp, equal(r, rp) );
        if(r >= mfirstrp && r<=mlastrp && equal(r, rp) )
        {
           return r;
        }
            r = reprtab[r];        
    }
    return 0;
}

int check_if(int type_if)
{

    int flag = 0;
    char str[30] = {"\0"};

    if(type_if == SH_IF )
    {
        return 0;
    }
    else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
    {
        int i = 0;
        while(letter() || digit())
        {
        str[i++] = curchar;
        m_nextch();
        printf ("Mc str = %s\n",str);
        }
        if( faind_ident(str))
        {
            flag = 1;
        }
        if(type_if == SH_IFDEF)
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

void end_line()
{
    while(curchar != '\n')
    {
        if(curchar == ' ' || curchar == '\t')
        {
            m_nextch();
        }
        else
        {
        printf("плохой символ %c %i\n", curchar, curchar);
        exit (10);
        }
    }
    m_nextch();
}

void folse_if ()
{
    int fl_cur;
   while(curchar != EOF)
    {
        if (curchar == '#')
        {
            printf("T31\n");
            fl_cur = macro_keywords();
            if(fl_cur == SH_ENDIF)
            {
                return;
            }
            if(fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                folse_if();
            } 
        }
        else
        {
        m_nextch();
        //printf("folse if c %c \n",curchar);
        }
    }
    printf("if не закончился1\n");
    exit (10);       
}

int m_folse()
{
    int fl_cur = cur;
    while(curchar != EOF)
    {
        if (curchar == '#')
        {
            fl_cur = macro_keywords();
            if(fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
            {
                return fl_cur;
            }
            if(fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                folse_if();
            } 
        }
        else
        {
            m_nextch();
        }
    }
    printf("if не закончился2\n");
    exit (10);      
}

void m_true(int type_if)
{
   while (curchar != EOF )
    {
       // printf ("c %c; ",curchar);
        macroscan();
        if (cur == SH_ELSE || cur == SH_ELIF )
        {
            break;
        }
        if(cur == SH_ENDIF)
        {
            return;
        }
    }

    if(type_if != SH_IF && cur == SH_ELIF)
    {
        printf("Неправильное макрослово true\n");
        exit (10);
    }
    
    folse_if();
   // printf("true c %c \n",curchar);
    return;
}

void m_if(int type_if)
{ 
    int flag = check_if(type_if);// начало (if)
    end_line();
    if(flag)
    {
        m_true(type_if);
        return;
    }
    else
    {
        cur = m_folse();
    }

    /*if (type_if == SH_IF) // середина (else if)
    {
        while (cur == SH_ELIF)
        {
            flag = check_if(type_if);
            end_line();
            if(flag)
            {
                m_true(type_if);
                return;
            }
            else
            {
                cur = m_folse();
            }

        }
    }
    else if (cur == SH_ELIF)
    {
        printf("Неправильное макрослово\n");
        exit (10);
    }*/

    if (cur == SH_ELSE)// конец (else)
    {
        cur = 0;
        m_true(type_if);
        return;
    }
}

void macroscan()
{  
    int j;
   // printf ("Макроскан\n");
    switch(curchar)
    {
        case EOF:
            return ;

        case '#':
            cur = macro_keywords ();

            if(cur == SH_DEFINE )
            {
                relis_define();
                m_nextch();
                return;
            }

            else if(cur == SH_IF || cur == SH_IFDEF || cur == SH_IFNDEF)
            {
                m_if(cur);
                return;
            }
            else if (cur == SH_ELSE || cur == SH_ELIF || cur == SH_ENDIF)
            {
                printf ("T11\n");
                return;
            }
            else
            {
            printf("Неправильное макрослово\n");
            exit (10);
            }
        case '/':
            r_coment();
            return;
        default:
            if(letter())
            {
                m_ident();
                for ( j = 0; j < msp; j++)
                {
                   m_fprintf(mstring[j]);  
                }
                return;
            }
            else
            {
                m_fprintf(curchar);
                m_nextch();

                return;
            }

    }
}


void preprocess_file()
{
    
    mfirstrp = rp;

    getnext();
    m_nextch();

    if (curchar == EOF)
    {
        printf ("Фаил не найден");
    }
   // printf ("Фаил найден\n");
    mlines[mline = 1] = 1;
    charnum = 1;
    mcl = 1;

    while (curchar != EOF )
    {
        //printf ("c %c; ",curchar);
        macroscan();
    }
    
}
