#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "global_vars.h"

#define STRIGSIZE 70

int macrotext [MAXREPRTAB];
int mstring[STRIGSIZE];
int fstring[STRIGSIZE];
int fchange[STRIGSIZE*2];
int localstack[STRIGSIZE];
int lsp = 0;
int cp = 0;
int fip = 0;
int fsp = 0;
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
int flag_from_macrotext = 1; // ??? временно

extern int getnext();
extern int letter();
extern int digit();
extern int equal();
extern void printf_char();
extern void fprintf_char();
extern void m_error();


int mletter();
int mdigit();
int mequal();

void mend_line();
void m_nextch();
void monemore();
void m_fprintf();

void to_macrotext();//
void macro_reprtab();
void from_macrotext();//5
int macro_keywords();//12
void relis_define();//2

void toreprtab_f();
int to_functionident (); //4
void scob();//6
void from_functionident();
void create_change();//11
void r_macrofunction();//3

//void m_ident();//5
int find_ident();

int check_if();//10
void end_line_space();//9
void false_if ();//8
int m_false();//7
void m_true();
void m_if();

void macroscan();//1,17
void preprocess_file();//18

void show_macro()
{
    int i1 = lines[line];
    int str1[STRIGSIZE];
    int j = 0;
    int k;
    int flag = 1;
    arg = mlines[m_conect_lines[line]];

    flag_show_macro = 1;
    while(i1 < charnum)
    {
        //printf("\nbe[arg= %i] = %i, so[i1] = %i",arg, before_source[arg],source[i1] );
        if(source[i1] == before_source[arg])
        {
            str1[j++]=before_source[arg];
            i1++;
            arg++;
        }
        else
        {
            flag = 0;
            curchar = before_source[arg];
            from_macrotext();

            i1 += msp;
        }
    }

    printf("line %i) ", m_conect_lines[line]);

    for (k = 0 ; k < j; k++)
    {
    printf_char(str1[k]);
    }
    if(flag == 0)
    {
      printf("\n В строке есть макрозамена, строка после макрогенерации:\nline %i)",m_conect_lines[line]);
        for ( k = lines[line-1]; k < charnum;k++)
        {
        printf_char(source[k]);
        }                  
    }
}

//простые
 int mletter(int r)
 {
    return (r >= 'A' && r <= 'Z') || (r >='a' && r <= 'z') 
    || r == '_' || (r >= 0x410/*А */ && r <= 0x44F /*'я'*/);
 }

 int mdigit(int r)
 {
    return r >='0' && r <= '9';
 }

 int mequal(int a)
 {
    int i = 0;
    int n = 1;  
    int j = 0;
    while (j < fip)
    {
        while (i < msp && mstring[a + i++] == fstring[j++]) {
            if (a + i == msp && fstring[j] == 0)
            {
                return n;
            }
        }
        n++;
        i = 0;
        if (fstring[j++] != 0)
            while (fstring[j++] != 0);    
    }

    return 0;
 }
//

// обработка символов
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
 }

 void m_nextch(int i)
 {
    //printf(" i = %d curcar = %c curcar = %i\n", i, curchar, curchar);
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
    }
    fprintf_char(output, a);
    return;
 }

 void end_line_space()
 {
    while(curchar != '\n')
    {
        if(curchar == ' ' || curchar == '\t')
        {
            m_nextch(9);
        }
        else
        {
            m_error(after_preproces_words_must_be_space);
        }
    }
    m_nextch(9);
 }
//

int find_ident()
{
    int fpr = rp;
    int i, r;
    hash = 0;
    fpr += 2;
    for( i = 0; i <msp; i++)
    {
        hash += mstring[i];
        reprtab[fpr++] = mstring[i];
    }
    reprtab[fpr++] = 0;
    hash &= 255;
    r = hashtab[hash];
    while(r)
    {
        if(r >= mfirstrp && r<=mlastrp && equal(r, rp) )
        {
           return r;
        }
            r = reprtab[r];        
    }
    return 0;
}

//define
 int macro_keywords() 
 {
    int oldrepr = rp;
    int r = 0;

    rp+=2;
    hash = 0;
                
    do
    {                 
    hash += curchar;
    reprtab[rp++] = curchar;
    m_nextch(12);
    }
    while(letter() || digit());

    if (curchar != ' ' && curchar != '\n' && curchar != '\t')
    {
        m_error(after_ident_must_be_space);
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
        if(equal(r, oldrepr))
        {
            rp = oldrepr;
            return(reprtab[r+1] < 0) ? reprtab[r+1] : (repr = r, IDENT);
        }
        else
        r = reprtab[r];
        }
        while(r);
    }
    return 0;
 }

 void to_reprtab()
 {
    int i;
    int r;
    int oldrepr = rp;

    mlastrp = oldrepr;
    hash = 0;
    rp += 2;

    do
    {
        hash += curchar;
        reprtab[rp++] = curchar;
        m_nextch(2);
    } while (letter() || digit());

    hash &= 255;
    reprtab[rp++] = 0;

    r = hashtab[hash];
    while(r)
    {
        if (equal(r, oldrepr))
        {
           m_error(repeat_ident);
        }

        r = reprtab[r];        
    }
    
    reprtab[oldrepr] = hashtab[hash];
    reprtab[oldrepr + 1] = mp;
    hashtab[hash] = oldrepr;
 }

 void to_macrotext()
 {

    m_nextch(2);

    macrotext[mp++] = 1;

    while(curchar != '\n')
    {
        macrotext[mp++] = curchar;
        m_nextch(2);

        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }

        if (curchar == '\\')
        {
            m_nextch(2);
            end_line_space();
        }
    }

    macrotext[mp++] = '\n';
 }

 void from_macrotext()
 { 
    int r;

    if(flag_from_macrotext)
    { 
        msp = 0;
    
        while(letter() || digit())
        {
            mstring[msp++] = curchar;
            m_nextch(5);
        }
    }
    flag_from_macrotext = 1;
    r = find_ident();
    
    if(r)
    {
        int t = reprtab[r + 1];
        msp = 0;
        if (macrotext[t] == 2)
        {
            from_functionident(t);
            return;
        }

        r = reprtab[r + 1] + 1;

        for( ; macrotext[r] != '\n'; r++)
        {
            mstring[msp++] = macrotext[r];
        }
    }
    
    return;
 }

 void relis_define()
 {
    if (letter() == 0) 
    { 
        m_error(ident_begins_with_letters);
    }

    to_reprtab();

    msp = 0;

    if (curchar == '(')
    { 
        m_nextch(2);
        r_macrofunction();
    }
    else if(curchar != ' ')
    {
        m_error(after_ident_must_be_space);
    }
    else
    {
        to_macrotext();   
    }

    return; 
 }
//

//define c параметрами
 int to_functionident()
 {
    int num = 0;
    fip = 0;
    while(curchar != ')')
    { 
        msp = 0; 
        if (letter()) 
        {
            while(letter() || digit())
            {
                fstring[fip++] = curchar;
                mstring[msp++] = curchar;
                m_nextch(4);
            }
            if (find_ident() != 0)
            {
                m_error(repeat_ident);
            }
            fstring[fip++] = 0;    
        }
        else 
        {
            m_error(functionid_begins_with_letters);
        }
        msp = 0;
        if(curchar == ',' && nextchar == ' ')
        {
            m_nextch(4);
            m_nextch(4); 
            num++;  
        }
        else if (curchar != ')')
        {
            m_error(after_functionid_must_be_comma);
        }
    }
    m_nextch(4);
    return num; 
 }

 void from_functionident(int t)
 {
    int i =  t + 2;
    int flag = 1;

    create_change(macrotext[t + 1]);

    msp = 0;
    while(macrotext[i] != '\n' ) 
    {
        if(macrotext[i] == -1)
        { 
            i++;
            int locp = localstack[macrotext[i] + lsp];
            i++;
            for(;fchange[locp] != '\n'; locp++)
            {
                mstring[msp++] = fchange[locp];
            }
            
        }
        else
        {
            mstring[msp++] = macrotext[i++];
        }
    }
    cp = 0;
 }

 void scob()
 {
    int i;
    fchange[cp++] = curchar;
    m_nextch(6);
    while(curchar != EOF)
    {
        if(letter())
        {
            int oldcp = cp;
            from_macrotext();
            cp = oldcp;

            for(i = 0; i < msp; i++)
            {
                fchange[cp++] =  mstring[i];
            }
        }
        else if(curchar == '(')
        {
            scob;
        }
        else
        {
            fchange[cp++] = curchar;
            m_nextch(6);
            if(curchar != ')')
            {
                fchange[cp++] = curchar;
                m_nextch(6);

                return;
            }
        }
    }
    m_error(scob_not_clous);
    return;
 }

 void create_change(int n)
 {
    int i;
    int num = 0;
    int oldlsp = lsp;
    localstack[num + lsp] = cp;

    if(curchar != '(')
    {
        m_error(stalpe);
    } 

    m_nextch(11);
    while(curchar != EOF)
    {
        if(letter())
        {
            int oldcp = cp;
            lsp = num + 1;
            from_macrotext();
            lsp = oldlsp;
            cp = oldcp;
            for(i = 0; i < msp; i++)
            {
                fchange[cp++] =  mstring[i];
            }
        }
        else if(curchar == '(')
        {
            scob();
        }
        else if(curchar != ')' && curchar != ',')
        {
        fchange[cp++] = curchar;
        m_nextch(11);
        }
        else
        {
            m_error(not_enough_param); // не та ошибка ????
        }

        if (curchar == ',' || curchar == ')')
        {
            fchange[cp++] = '\n';
            if(curchar == ',')
            {   
                num++;
                localstack[num + lsp] = cp;
                if(num > n)
                {
                    m_error(not_enough_param);
                }

                m_nextch(11);
                if(curchar == ' ')
                    m_nextch(11);
            }
            else
            {
                if(num != n)
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

 void funktionleter()
 {
    int n;
    int i;
    msp = 0;
    while(letter() || digit())
    {
        mstring[msp++] = curchar;
        m_nextch(5);
    } 
    if((n = mequal(0)) != 0)
    {
        macrotext[mp++] = -1;
        macrotext[mp++] = n-1;
    } 
    else
    {
        int i = 0;
        flag_from_macrotext = 0;
        from_macrotext(); // не совсем ????
        flag_from_macrotext = 1;

        while(i < msp)
        {    
            if(mletter(mstring[i]) || mdigit(mstring[i]))
            {
                int oldmsp = msp;
                int k = 0;
                while(i < oldmsp && (mletter(mstring[i]) || mdigit(mstring[i])))
                {
                    mstring[msp++] = mstring[i];
                    i++;
                } 

                if((n = mequal(oldmsp)) != 0)
                {
                    macrotext[mp++] = -1;
                    macrotext[mp++] = n-1;
                }
                else
                {
                    for(int k = oldmsp; k < msp; k++)
                    {
                        macrotext[mp++] = mstring[k];
                    }
                }
                msp = oldmsp;   
            }
            else
            {
                macrotext[mp++] = mstring[i++]; 
            }
        }  
    }
    msp = 0;  
 }

 void r_macrofunction()
 {
    int j;

    macrotext[mp++] = 2;
    macrotext[mp++] = to_functionident();
    m_nextch(3);

    while(curchar != '\n')
    {
        ///printf("c1 = %c\n", curchar);
        if (letter())
        {    
            funktionleter();
        }
        else
        {
        macrotext[mp++] = curchar;
        m_nextch(3);
        }

        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }

        if (curchar == '\\')
        {
            m_nextch(3);
            end_line_space();
        }
    }
    macrotext[mp++] = '\n';
    return;
 }
//

//if
 int check_if(int type_if)
 {

    int flag = 0;

    if(type_if == SH_IF )
    {
        m_error(not_relis_if);
    }
    else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
    {
        msp = 0;
        while(letter() || digit())
        {
        mstring[msp++] = curchar;
        m_nextch(10);
        }
        if( find_ident())
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

 void false_if ()
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
                false_if();
            } 
        }
        else
        {
        m_nextch(8);
        }
    } 
    m_error(must_be_endif);
 }

 int m_false()
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
                false_if();
            } 
        }
        else
        {
            m_nextch(7);
        }
    }  
    m_error(must_be_endif);
    return 1;
 }

 void m_true(int type_if)
 {
   while (curchar != EOF )
    {
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
    
    false_if();
    return;
 }

 void m_if(int type_if)
 { 
    checkif++;
    int flag = check_if(type_if);// начало (if)
    end_line_space();
    if(flag)
    {
        m_true(type_if);
        return;
    }
    else
    {
        cur = m_false();
    }

    /*if (type_if == SH_IF) // 
    {
        while (cur == SH_ELIF)
        {
            flag = check_if(type_if);
            end_line_space();
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

    if (cur == SH_ELSE)// 
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
//

void macroscan()
{  
    int j;
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
            m_fprintf(curchar);
            m_nextch(17);
            return;
            m_error(preproces_words_not_exist);
            }

        case '\'':
                m_fprintf(curchar);
                m_nextch(17);
                if(curchar == '\\')
                {
                    m_fprintf(curchar);
                    m_nextch(17); 
                }
                m_fprintf(curchar);
                m_nextch(17);
                
                m_fprintf(curchar);
                m_nextch(17);
                return;

        case '\"':
                m_fprintf(curchar);
                m_nextch(17);
            while(curchar != '\"' && curchar != EOF)
            {
                if(curchar == '\\')
                {
                    m_fprintf(curchar);
                    m_nextch(17);  
                }
                m_fprintf(curchar);
                m_nextch(17);
            }
            m_fprintf(curchar);
            m_nextch(17);
            return;
        default:
            if(letter() && prep_flag == 1)
            {
                from_macrotext();
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
    mlines[mline = 1] = 1;
    charnum = 1;
    mcl = 1;

    getnext();
    m_nextch(18);
    while (curchar != EOF )
    {
        macroscan();
    }
    m_conect_lines[mcl++] = mline-1;
}

