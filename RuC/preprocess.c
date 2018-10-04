#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "global_vars.h"


int macrotext [MAXREPRTAB];
char mstring[50];
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
int checkif = 0;
int flag_show_macro = 0;
int arg = 0;

extern int getnext();
extern int letter();
extern int digit();
extern int equal();
extern void printf_char(int wchar);
extern void m_error(int ernum);


int mletter(int r);
int mdigit(int r);
int mequal(int str[], int j);

void mend_line();
void m_nextch();
void m_fprintf(int a);

void to_macrotext(char chang[], int oldrepr);//
void macro_reprtab(char str[],char chang []);
void from_macrotext(char str[]);
int macro_keywords();//12
void relis_define();//2

void toreprtab_f(char str[]);
void to_functionident (); //4
int scob(int cp);//6
void from_functionident(int r);
void create_chenge(int r1);//11
void r_macrofunction();//3

void m_ident();//5
int faind_ident(char str[]);

int check_if(int type_if);//10
void end_line();//9
void folse_if ();//8
int m_folse();//7
void m_true(int type_if);
void m_if(int type_if);

void macroscan();//1,17
void preprocess_file();//18

void show_macro()
{
    int i1 = lines[line];
    char str1[50];
    int j = 0;
    int k;
    int flag=1;
    arg = mlines[m_conect_lines[line]];

    flag_show_macro = 1;
    while(i1 < charnum)
    {
        if(source[i1] == before_source[arg])
        {
            str1[j++]=before_source[arg];
            i1++;
            arg++;
        }
        else
        {
            flag=0;
            curchar = before_source[arg];
            m_ident();

            i1 += msp;
        }
    }

    printf("line %i) ", m_conect_lines[line]);

    for (k = 1; k < j;k++)
    {
    printf_char(str1[k]);
    }
    if(flag == 0)
    {
      printf("В строке есть макрозамена, строка после макрогенерации:\nline) ");
        for ( k = lines[line-1]; k < charnum;k++)
        {
        printf_char(source[k]);
        }                  
    }
}

int mletter(int r)
{
    return (r >= 'A' && r <= 'Z') || (r >='a' && r <= 'z') 
    || r == '_' || (r >= 0x410/*А */ && r <= 0x44F /*'я'*/);
}

int mdigit(int r)
{
    return r >='0' && r <= '9';
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

void mend_line()
{
    int j;
    if (flag_show_macro == 0)
   {
        mlines[++mline] = m_charnum;
        mlines[mline+1] = m_charnum;
        if (kw)
        {
            printf("Line %i) ", mline-1);
            for ( j=mlines[mline-1]; j<mlines[mline]; j++)
                if (before_source[j] != EOF)
                    printf_char(before_source[j]);
        } 
   }

        return;
}

void monemore()
{
    if(flag_show_macro == 0)
    {
        curchar = nextchar;
        nextchar = getnext();
        before_source[m_charnum++] = curchar;
    }
    else
    {
    curchar = before_source[arg++];
    }

    if (curchar == EOF)
    {
        mend_line();
        printf("\n");
        return;
    } 

    //before_source[m_charnum++] = curchar; 

}

void m_nextch(int i)
{
    // printf(" %i ",i);
    monemore();

    if (curchar == '/' && nextchar == '/')
    {
        if(i>13)
        {
            m_fprintf(curchar);
        }
        do
        {
            monemore();
            if(i>13)
            {
                m_fprintf(curchar);
            }
        }
        while (curchar != '\n');
        
        mend_line();
        return;
    }

    if (curchar == '/' && nextchar == '*')
    {
        if(i>13)
        {
            m_fprintf(curchar);
        }
    
        monemore();
        if(i>13)
        {
            m_fprintf(curchar);
        }
        do
        {
            monemore();
            if(i>13)
            {
                m_fprintf(curchar);
            }
            
            if (curchar == EOF)
            {
                mend_line();
                printf("\n");
                m_error(comm_not_ended);
            }
            if (curchar == '\n')
                mend_line();
        }
        while (curchar != '*' || nextchar != '/');
        
        monemore();
        if(i>13)
        {
            m_fprintf(curchar);
        }
        curchar = ' ';
        return;
    }
        
    if (curchar == '\n')
    {
     mend_line();
    }
    return;
}

void m_fprintf(int a)
{
    if(a == '\n')
    {
    m_conect_lines[mcl++] = mline-1;
    //printf("m_c_line[%i]=%i) ", mcl-1,m_conect_lines[mcl-1]);
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
            r = reprtab[r];
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
    m_nextch(12);
    //printf ("c %c; ",curchar);
    }
    while(letter() || digit());

    if (curchar != ' ' && curchar != '\n' && curchar != '\t')
    {
        m_error(after_ident_mast_be_spase);
    }
    else
    {
      m_nextch(12);
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
            m_nextch(2);
        }
        
        if (faind_ident(str) != 0)
        {
            m_error(repeat_ident);
        }

        if (curchar == '(')
        { 

            toreprtab_f(str);
            //printf("замена на функцию\n");
            m_nextch(2);
            r_macrofunction();
            return;
        }
        else if(curchar != ' ')
        {
            m_error(after_ident_mast_be_spase);
        }
        else
        {
            //printf("макрозамена\n");
            m_nextch(2);
            i=0;
            char chang [30] = {"\0"};
            while(curchar != '\n')
            {
                chang[i++] = curchar;
                m_nextch(2);
                if(curchar == EOF)
                {
                    m_error(not_end_fail_preprocess);
                }
            }
            macro_reprtab(str, chang);
            return;
        }
    }
    else
    { 
        m_error(ident_begins_with_leters);
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
        char str[30]="\0";                             //reportab
        int i=0;                                       //   \/
        fip++;                                     //funcid 5[] -> конец = 13
        if (letter())                              //       6[] -> macrofunc
        {                                          //       7[] -> fcang 
            i = 0;                                 //       8[a]
            while(letter() || digit())             //       9[0]
            {                                      //      10[] -> fcang
                functionident[fip++] = curchar;    //      11[b] 
                str[i++] = curchar;                //      12[0]
               // printf("'%c'",curchar);
                m_nextch(4);
            }
            if (faind_ident(str) != 0)
            {
                m_error(repeat_ident);
            }
            functionident[fip++] = 0;    
        }
        else 
        {
            m_error(functionid_begins_with_leters);
        }

        if(curchar == ',' && nextchar == ' ')
        {
            m_nextch(4);
            m_nextch(4); 
           // printf("',' ' '");  
        }
        else if (curchar != ')')
        {
            m_error(after_functionid_mact_be_coma);
        }
    }

   // printf("\n");
    m_nextch(4);
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
        if(mletter(macrofunction[newfi]))
        {
            flag = 1;
            for ( i = 0; i < 30; i++)
            {
                str[i] = 0;
            }
            i = 0;
            while(mletter(macrofunction[newfi]) || mdigit(macrofunction[newfi]))
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

int scob(int cp)
{
    int i;
    fchange[cp++] = curchar;
    m_nextch(6);
    while(curchar != EOF)
    {
        if(letter())
        {
            m_ident();
            for(i = 0; i < msp; i++)
            {
               fchange[cp++] =  mstring[i];
            }
        }
        else if(curchar == '(')
        {
            cp = scob(cp);
        }
        else
        {
            fchange[cp++] = curchar;
            m_nextch(6);
            if(curchar != ')')
            {
                fchange[cp++] = curchar;
                m_nextch(6);
                 return cp;
            }
        }
    }
    m_error(scob_not_clous);
    return cp;
}
void create_chenge(int r1)
{
    int i;
    int r = r1 + 2;
    int cp = 1;
    functionident[r] = cp;
    //for(int q=0;q+r1<functionident[r1];q++);
    //printf("r1 = %i, fi[r1+q = %i] ='%d'\n ",r1, r1+q, functionident[r1+q]);
    if(curchar == '(')
    { 
        m_nextch(11);
        while(curchar != EOF)
        {

            if(letter())
            {
                m_ident();
                for(i = 0; i < msp; i++)
                {
                    fchange[cp++] =  mstring[i];
                }
            }
            else if(curchar == '(')
            {
                cp = scob(cp);
            }
            else if(curchar != ')' && curchar != ',')
            {
            fchange[cp++] = curchar;
            //printf("'%c'",curchar);
            m_nextch(11);
            }
            else
            {
               m_error(not_enough_param); 
            }

            if (curchar == ',' || curchar == ')')
            {
                for(; functionident[r] != 0; r++);

                if(r < functionident[r1])
                {
                    fchange[cp++] = '\n';
                    r++;
                }
                else
                {
                    m_error(not_enough_param);
                }

                if(curchar == ',')
                {
                    functionident[r] =cp;
                    m_nextch(11);
                }
                else
                {
                    if(r != functionident[r1])
                    {
                        m_error(not_enough_param2);

                    }
                    m_nextch(11);
                    return;
                }
            }
            
        }
        m_error(scob_not_clous);
    } 
    else
    {
        m_error(stalpe);
    }
}

void r_macrofunction()
{
    int j;
    int olderfip = fip++;
    functionident[fip++] = mfp;

   // printf("'%c'\n",curchar);
    to_functionident ();
    m_nextch(3);
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
        m_nextch(3);
        }

        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }

    }
    macrofunction [mfp++] = '\n';
    return;
}


void m_ident()
{
    int i = 0;
    char str[50] = {"\0"};
    while(letter() || digit())
    {
        str[i++] = curchar;
        m_nextch(5);
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
        m_error(not_relis_if);
    }
    else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
    {
        int i = 0;
        while(letter() || digit())
        {
        str[i++] = curchar;
        m_nextch(10);
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
            m_nextch(9);
        }
        else
        {
            m_error(after_preproces_words_mast_be_spase);
        }
    }
    m_nextch(9);
}

void folse_if ()
{
    int fl_cur;
   while(curchar != EOF)
    {
        if (curchar == '#')
        {
            fl_cur = macro_keywords();
            if(fl_cur == SH_ENDIF)
            {
                checkif--;
                if(checkif == -1)
                {
                    m_error(befor_endif);
                }
                return;
            }
            if(fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                checkif++;
                folse_if();
            } 
        }
        else
        {
        m_nextch(8);
        //printf("folse if c %c \n",curchar);
        }
    } 
    m_error(mast_be_endif);      
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
            m_nextch(7);
        }
    }  
    m_error(mast_be_endif);    
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
            checkif--;
            if(checkif == -1)
            {
                m_error(befor_endif);
            }
            return;
        }
    }

    if(type_if != SH_IF && cur == SH_ELIF)
    {
        m_error(dont_elif);
    }
    
    folse_if();
   // printf("true c %c \n",curchar);
    return;
}

void m_if(int type_if)
{ 
    checkif++;
    printf("check_if = %i",checkif);
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

    if(cur == SH_ENDIF)
    {
        checkif--;
        if(checkif == -1)
        {
            m_error(befor_endif);
        }
    }
}

void macroscan()
{  
    int j;
    //printf ("Макроскан\n");
    switch(curchar)
    {
        case EOF:
            return ;

        case '#':
            cur = macro_keywords ();
            prep_flag = 1;

            if(cur == SH_DEFINE )
            {
                relis_define();
                m_nextch(1);
                return;
            }

            else if(cur == SH_IF || cur == SH_IFDEF || cur == SH_IFNDEF)
            {
                m_if(cur);
                return;
            }
            else if (cur == SH_ELSE || cur == SH_ELIF || cur == SH_ENDIF)
            {
                return;
            }
            else
            {
            m_error(preproces_wods_not_exist);
            }
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
                m_nextch(17);

                return;
            }

    }
}


void preprocess_file()
{
    
    mfirstrp = rp;

    getnext();
    m_nextch(18);

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
    m_conect_lines[mcl++] = mline-1;
    
}