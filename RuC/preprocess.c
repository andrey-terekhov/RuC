#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "global_vars.h"

#define STRIGSIZE 70
#define DIP 4

int macrotext [MAXREPRTAB];
int mp = 0;

int mstring[STRIGSIZE];
int msp = 0;

//int fstring[STRIGSIZE];
//int fsp = 0;
//int oldfsp = 0;

int fchange[STRIGSIZE*2];
int cp = 0;

int localstack[STRIGSIZE];
int lsp = 0;

int cstring[STRIGSIZE];
int csp = 0;

int ifgstring[STRIGSIZE];
int gifp = 0;
//int oldgifp = 0;

int mfirstrp = -1; // начало и конец макрослов в reprtab
int mlastrp = -1;

int mclp = 1;
int checkif = 0;
int flagint = 1;

int nextp = 0;
int nextch_type = 5;
int nextch_stop = 0;

int oldcurchar[DIP];
int oldnextchar[DIP];
int oldnextch_type[DIP];
int oldnextp[DIP];
int dipp;

extern int getnext();
extern int letter();
extern int digit();
extern int equal();
extern void printf_char();
extern void fprintf_char();
extern void m_error();


void show_macro()
{
    int i1 = lines[line];
    int str1[STRIGSIZE];
    int j = 0;
    int k;
    int flag = 1;
    nextp = mlines[m_conect_lines[line]];
    m_change_nextch_type(1);


    while(i1 < charnum)
    {
        //printf("\nbe[arg= %i] = %i, so[i1] = %i",arg, before_source[arg],source[i1] );
        if(source[i1] == curchar)
        {
            str1[j++] = curchar;
            m_nextch();
            i1++;
        }
        else
        {
            flag = 0;
            define_get_from_macrotext();
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

//простые (m)

 void m_change_nextch_type(int type)
 {
    if (type != 1)
    {
        oldcurchar[dipp] = curchar;
        oldnextchar[dipp] = nextchar;
        oldnextch_type[dipp] = nextch_type;
        oldnextp[dipp] = nextp;
        nextp = 0;
        dipp++;
    }
    else
    {
        dipp = 0;
    }
    

    nextch_type = type;
    m_nextch();
 }

 void m_old_nextch_type()
 {
    dipp--;
    curchar = oldcurchar[dipp];
    nextchar = oldnextchar[dipp];
    nextch_type = oldnextch_type[dipp];
    nextp = oldnextp[dipp];
 }
 int m_letter(int r)
 {
    return (r >= 'A' && r <= 'Z') || (r >='a' && r <= 'z') 
    || r == '_' || (r >= 0x410/*А */ && r <= 0x44F /*'я'*/);
 }

 int m_digit(int r)
 {
    return r >='0' && r <= '9';
 }

 int  m_equal(int a)
 {
    int i = 0;
    int n = 1;  
    int j = 0;
    while (j < csp)
    {
        while (i < msp && mstring[a + i++] == cstring[j++]) {
            if (a + i == msp && cstring[j] == 0)
            {
                return n;
            }
        }
        n++;
        i = 0;
        if (cstring[j++] != 0)
            while (cstring[j++] != 0);    
    }

    return 0;
 }

 int m_ispower(int r)
 {
    return r == 'e' || r == 'E'; // || r == 'е' || r == 'Е') // это русские е и Е
 }
//

//базовая обработка символов (m)
 void m_end_line()
 {
    int j;
   
    mlines[++mline] = m_charnum;
    mlines[mline+1] = m_charnum;

    printf("Line %i) ", mline-1);
    for ( j=mlines[mline-1]; j<mlines[mline]; j++)
        if (before_source[j] != EOF)
            printf_char(before_source[j]);

    return;
 }

 void m_onemore()
 {
    
    curchar = nextchar;
    nextchar = getnext();
    before_source[m_charnum++] = curchar;

    if (curchar == EOF)
    {
        m_end_line();
        printf("\n");
        return;
    } 
 }

  void m_fprintf(int a)
 {
    if(a == '\n')
    {
    m_conect_lines[mclp++] = mline-1;
    }
    fprintf_char(output, a);
  //   printf_char(a);
  //   printf(" %d \n", a);
    return;
 }

 void m_fprintf_com()
 {
    if(nextch_type != 17)
    {
        m_fprintf(curchar);
    }
 }

 void m_coment_skip()
 {
    if (curchar == '/' && nextchar == '/')
    {
        do
        {
            m_fprintf_com();
            m_onemore();
        }
        while (curchar != '\n');
    }
    if (curchar == '/' && nextchar == '*')
    {
        m_fprintf_com();
        m_onemore();
        m_fprintf_com();
        do
        {
            m_onemore();
            m_fprintf_com();
            
            if (curchar == EOF)
            {
                m_end_line();
                printf("\n");
                m_error(comm_not_ended);
            }
        }
        while (curchar != '*' || nextchar != '/');
        
        m_onemore();
        m_fprintf_com();
        curchar = ' ';
    }  
 }
 

 void m_nextch()
 {
    //printf(" i = %d curcar = %c curcar = %i\n", nextch_type, curchar, curchar);

    if(nextch_type < 3)
    {
        if(nextch_type == 1)
        {
            curchar = before_source[nextp++];
            nextchar = before_source[nextp];
            return;
        }
        else if(nextch_type == 2 && nextp < msp)
        {
            curchar = mstring[nextp++];
            nextchar = mstring[nextp];
            return;
        }
        else
        {
            m_old_nextch_type();
        }
    }    
    else
    {
        m_onemore();
        m_coment_skip();    
        if (curchar == '\n')
        {
            m_end_line();
        }
    }
    return;
 }
//

// обработка пробелов (space)
 void space_end_line()
 {
    while(curchar != '\n')
    {
        if(curchar == ' ' || curchar == '\t')
        {
            m_nextch();
        }
        else
        {
            m_error(after_preproces_words_must_be_space);
        }
    }
    m_nextch();
 }

 void space_skip()
 {
    while(curchar == ' ' || curchar == '\t')
        {
            m_nextch();
        }
 }
//

/* /работа сo строками

 void collect_scob()
 {
    int n = 1;
    fsp = oldfsp;
    fstring[fsp++] = curchar;
    m_nextch();
    while (curchar != EOF)
    {
        if(curchar == '(')
        {
            n++;
            fstring[fsp++] = curchar;
            m_nextch();
        }
        else if (curchar == ')')
        {
            n--;
            fstring[fsp++] = curchar;
            m_nextch();
        }
        else
        {
            fstring[fsp++] = curchar;
            m_nextch();
        }
        
        if(n == 0)
        {
            define_get_from_macrotext();

            return;
        } 
    }
    m_error(scob_not_clous);

    return;
 }

 int s_collect_scob(int str[], int i)
 {
    int n = 1;
    fsp = oldfsp;
    fstring[fsp++] = str[i++];

    while (str[i] != EOF)
    {
        if(str[i] == '(')
        {
            n++;
            fstring[fsp++] = str[i++];
        }
        else if (str[i] == ')')
        {
            n--;
            fstring[fsp++] = str[i++];
        }
        else
        {
            fstring[fsp++] = str[i++];
        }
        
        if(n == 0)
        {
            define_get_from_macrotext();

            return i;
        } 
    }
    m_error(scob_not_clous);

    return i;
 }

 void collect_mident()
 {
    msp = 0;
    
    while(letter() || digit())
    {
        mstring[msp++] = curchar;
        m_nextch();
    }

    if(curchar == '(')
    {
      collect_scob();
    }
    else
    {
        define_get_from_macrotext();
    }

    return;
 }

 int s_collect_mident(int str[], int i)
 {
    msp = 0;
    while(m_letter(str[i]) || m_digit(str[i]))
    {
        mstring[msp++] = str[i++];
    }
    if(str[i] == '(')
    {
      i = s_collect_scob(str, i);
    }
    else
    {
        define_get_from_macrotext();
    }

    return i;
 }
/*/

 void collect_mident()
 {
    msp = 0;
    
    while(letter() || digit())
    {
        mstring[msp++] = curchar;
        m_nextch();
    }

    return;
 }

// macro
 int macro_find_ident()
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
    m_nextch();
    }
    while(letter() || digit());

    if (curchar != '\n' && curchar != ' ' && curchar != '\t' && curchar != '(')
    {
        m_error(after_ident_must_be_space);
    }
    else if(curchar != '(')
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
        if(equal(r, oldrepr))
        {
            rp = oldrepr;
            return(reprtab[r+1] < 0) ? reprtab[r+1] : 0;
        }
        else
        r = reprtab[r];
        }
        while(r);
    }
    rp = oldrepr;
    return 0;
 }

 /*int macro_keywords_eval(int str[], int i)
 {
    int oldrepr = rp;
    int r = 0;

    rp+=2;
    hash = 0;
                
    do
    {                 
    hash += str[i];
    reprtab[rp++] = str[i++];
    m_nextch();
    }
    while(letter() || digit());
                
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
            if(reprtab[r+1] < 0)
                cur = reprtab[r+1];
            else 
                cur = 0;
            return i;
        }
        else
        r = reprtab[r];
        }
        while(r);
    }
    rp = oldrepr;
    return i;
 }*/

//

/* /eval
 void eval_relis()
  {
    int count = 0;
    csp = 0;

    if (cur != SH_EVAL)
    { 
        int r = rp;
        while (reprtab[r] != 0)
        {
            cstring[csp++] = reprtab[r++];
        }
        
        return;
    }

    //if(curchar != '(') ошибка

    while(count != -1)
    {
        if (letter())
        {
            from_macrotext();
            for ( j = 0; j < msp; j++)
            {
                cstring[csp++] =  mstring[j];  
            }
            msp = 0;
        }
        else if(curchar == '(')
        {
            count++;
            cstring[csp++] =  curchar;
        }
        else if(curchar == ')')
        {
            count--;
            cstring[csp++] =  curchar;
        }
        else //curchar == '\n' ошибка
        {
            cstring[csp++] =  curchar;
        }
    }
   calculator(); 
  }

  double take_digit()
  {
    int j;
    csp = 0;
    if(digit() || ifgstring[gifp] == '-')
    {
        do 
        {
            cstring[csp++] = ifgstring[gifp++];

        } while (m_digit(ifgstring[gifp]) || '.' ||m_ispower(ifgstring[gifp]));
    }
    else if(m_letter(ifgstring[gifp]))
    {
        gifp = s_collect_mident(ifgstring, gifp);
        from_macrotext();
        for (j = 0; j < msp; j++)
        {
           cstring[csp++] = mstring[j];  
        }
    }
    else
    {
        gifp = macro_keywords_eval(ifgstring, gifp);
        //if(ifgstring[j] != '(') ошибка
        eval_relis()
    }

    while (ifgstring[j] == ' '||ifgstring[j] == '\t')
    {
        j++;
    }

    return get_digit(0);   
  }
 /*/

 double get_digit()
 {
    int flagtoolong = 0;
    double k;
    int d = 1;
    flagint = 1;
    num = 0;
    numdouble = 0.0;
    if(curchar == '-')
    {
        d = -1;
        m_nextch();
    } 

    while (digit())
    {
        numdouble = numdouble * 10 + (curchar - '0');
        if (numdouble > (double)INT_MAX)
        {
            flagtoolong = 1;
            flagint = 0;
        }
        num = num * 10 + (curchar - '0');
        m_nextch();// если выйдит за рамки
    }

    if (curchar == '.')
    {
        flagint = 0;
        m_nextch();
        k = 0.1;
        while (digit())
        {
            numdouble += (curchar - '0') * k;
            k *= 0.1;
            m_nextch();
        }
    }

    if (ispower())
    {
        int d = 0, k = 1, i;
        m_nextch();
        if (curchar == '-')
        {
            flagint = 0;
            m_nextch();
            k = -1;
        }
        else if (curchar == '+')
        {
            m_nextch();
        }

        //if (!digit())
        //{
        //    m_error(must_be_digit_after_exp);// сделать 
        //}

        while (digit())
        {
            d = d * 10 + curchar - '0';
            m_nextch();
        }
        if (flagint)
        {
            for (i = 1; i <= d; i++)
                num *= 10;
        }
        numdouble *= pow(10.0, k * d);  
    }

    if (flagint)
    {   
        return num * d;
    }
    else
    {
        return numdouble * d;
    } 
  
 }

 int check_opiration()
 {
    int c = curchar;
    switch(c)
    {
        case '|':
                if(nextchar == '|')
                {
                    m_nextch();
                    m_nextch();
                    return c;
                }
                else
                {
                    return 0;
                }
        case '&':
                if(nextchar == '&')
                {
                    m_nextch();
                    m_nextch();
                    return c;
                }
                else
                {
                    return 0;
                }
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '(':
            m_nextch();
            return c;
        default:
            return 0;
    }
 }

 int get_prior(int r)
 {
    switch(r)
    {
        case '(':
            return 0;
        case '|':
            return 1;
        case '&':
            return 2;
        case '+':
            return 3;
        case '-':
            return 3;
        case '*':
            return 4;
        case '/':
            return 4;
        case '%':
            return 4;
        default:
            return -1;
        //ошибка
    }
 }

 double relis_opiration(double x, double y, int r,int int_flag)
 {
    switch(r)
    {
        case '&':
            return x && y;
        case '|':
            return x || y;
        case '+':
            return x + y;
        case '-':
            return x - y;
        case '*':
            return x * y;
        case '/':
                if(int_flag)
                {
                   return (int)x / (int)y;// нужно ли ???
                }
                else
                {
                    return x / y;
                }
        case '%':
                if(int_flag)
                {
                    return (int)x % (int)y;
                }
        default:
            return 0;//ошибка
    }
 }

 void double_to_string(double x, int int_flag)
 {
    char s[30];
    if(int_flag)
    {
        sprintf(s,"%f",x);
        for(csp = 0; csp < 20; csp++)
        {
            cstring[csp] = s[csp];
            if(s[csp] == '.')
                return;
        }    
    }
    else
    {
        int l;
        sprintf(s,"%.14lf",x);
        for(csp = 0; csp < 20; csp++)
        {
            cstring[csp] = s[csp];
            if(s[csp] != '0' && m_digit(s[csp]))
                l = csp;
        }
        csp = l+1;
    }
 }

 void calculator()
 {
    int i = 0; 
    int op = 0;
    int c;
    double stack[10];
    int int_flag = 1;
    int operation[10];

    operation[op++] = '(';
    m_nextch();

    while(curchar != '\n')
    {
        space_skip();

        if(digit() || curchar == '-')
        {
            stack[i++] = get_digit();
            int_flag = flagint && int_flag;
        } 
        else if (letter() && nextch_type != 2)
        {
            collect_mident();
            define_get_from_macrotext();
            m_change_nextch_type(2);
        }
        else if((c = check_opiration()))
        { 
            while(op != 0 && get_prior(c) != 0 && get_prior(operation[op-1]) >= get_prior(c))
            {
                stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op-1], int_flag);
                op--;
                i--;
            }
            operation[op++] = c;   
        }
        else if(curchar == ')')   
        { 
            while(operation[op-1] != '(')
            {
                //if(i < 2 ||op == 0) ошибка
                stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op-1], int_flag);
                op--;
                i--;
            }
            op--;
            m_nextch();
            if(op == 0)
            {
                double_to_string(stack[0], int_flag); 
                return;
            }
        }//else ошибка
    }

    while (op > 0)
    {
        //if(i < 2) ошибка
        stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op-1], int_flag);
        op--;
        i--;
    }
    
    double_to_string(stack[0], int_flag); 
 }
//

//define c параметрами (function)
 void function_scob_collect()
 {
    int i;
    fchange[cp++] = curchar;
    m_nextch();
    while(curchar != EOF)
    {
        if(letter())
        {
            int oldcp = cp;
            //int old_loc_fsp = oldfsp;
            int oldlsp = lsp;
            //oldfsp = fsp;
            lsp = num + 1;
            //j = s_collect_mident(fstring, j);
            define_get_from_macrotext();
            //fsp = oldfsp;
            //oldfsp = old_loc_fsp;
            lsp = oldlsp;
            cp = oldcp;
            for(i = 0; i < msp; i++)
            {
                fchange[cp++] =  mstring[i];
            }
        }
        else if(curchar == '(')
        {
            fchange[cp++] = curchar;
            m_nextch();
            function_scob_collect();
        }
        else
        {
            fchange[cp++] = curchar;
            m_nextch();
            if(curchar != ')')
            {
                fchange[cp++] = curchar;
                m_nextch();

                return;
            }
        }
    }
    m_error(scob_not_clous);
    return;
 }

 void function_stack_create(int n)
 {
    int i;
    int num = 0;
    fchange[cp++] = curchar;
    m_nextch();
    localstack[num + lsp] = cp;

    if(curchar == ')')
    {
        m_error(stalpe);
    } 

    
    while(curchar != ')')
    {
        if(letter())
        {
            int oldcp = cp;
            //int old_loc_fsp = oldfsp;
            int oldlsp = lsp;
            //oldfsp = fsp;
            lsp = num + 1;
            //j = s_collect_mident(fstring, j);
            collect_mident();
            define_get_from_macrotext();
            //fsp = oldfsp;
            //oldfsp = old_loc_fsp;
            lsp = oldlsp;
            cp = oldcp;

            for(i = 0; i < msp; i++)
            {
                fchange[cp++] =  mstring[i];
            }
        }
        else if(curchar == '(')
        {
            fchange[cp++] = curchar;
            m_nextch();
            function_scob_collect();
        }
        else if(curchar != ')' && curchar != ',')
        {
            fchange[cp++] = curchar;
            m_nextch();
        }
        else
        {
            m_error(not_enough_param); // не та ошибка ????
        }


        if (curchar == ',')
        {
            fchange[cp++] = '\n'; 
            num++;
            localstack[num + lsp] = cp;
            if(num > n)
            {
                m_error(not_enough_param);
            }

            m_nextch();
            if(curchar == ' ')
                m_nextch();
        }    
        else if (curchar == ')')
        {
            fchange[cp++] = '\n';
            if(num != n)
            {
                m_error(not_enough_param2);
            }
            m_nextch();
            //fsp = oldfsp;
            return;
        }       
    }
    m_error(scob_not_clous);    
 }

 void function_get_from_macrotext(int t)
 {
    int i =  t + 2;
    int flag = 1;

    function_stack_create(macrotext[t + 1]);

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

 void funktionleter()
 {
    int n;
    int i;
    msp = 0;
    collect_mident();

    if((n = m_equal(0)) != 0)
    {
        macrotext[mp++] = -1;
        macrotext[mp++] = n-1;
    } 
    else
    {
        int i = 0;

        define_get_from_macrotext();

        while(i < msp)
        {    
            if(m_letter(mstring[i]) || m_digit(mstring[i]))
            {
                int oldmsp = msp;
                int k = 0;
                while(i < oldmsp && (m_letter(mstring[i]) || m_digit(mstring[i])))
                {
                    mstring[msp++] = mstring[i];
                    i++;
                } 

                if((n = m_equal(oldmsp)) != 0)
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

 int to_functionident()
 {
    int num = 0;
    csp = 0;
    while(curchar != ')')
    { 
        msp = 0; 
        if (letter()) 
        {
            while(letter() || digit())
            {
                cstring[csp++] = curchar;
                mstring[msp++] = curchar;
                m_nextch();
            }
            if (macro_find_ident() != 0)
            {
                m_error(repeat_ident);
            }
            cstring[csp++] = 0;    
        }
        else 
        {
            m_error(functionid_begins_with_letters);
        }
        msp = 0;
        if(curchar == ',')
        {
            m_nextch();
            space_skip();
            num++;  
        }
        else if (curchar != ')')
        {
            m_error(after_functionid_must_be_comma);
        }
    }
    m_nextch();
    return num; 
 }

 void function_add_to_macrotext()
 {
    int j;

    macrotext[mp++] = 2;
    macrotext[mp++] = to_functionident();
    m_nextch();

    while(curchar != '\n')
    {

        if (letter())
        {    
            funktionleter();
        }
        else
        {
        macrotext[mp++] = curchar;
        m_nextch();
        }

        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }

        if (curchar == '\\')
        {
            m_nextch();
            space_end_line();
        }
    }
    macrotext[mp++] = '\n';
    return;
 }
//

//define
 void define_get_from_macrotext()
 { 
    int r = macro_find_ident();
    int j;
    
    if(r)
    {
        int t = reprtab[r + 1];
        msp = 0;
        if (macrotext[t] == 2)
        {
            function_get_from_macrotext(t);
            return;
        }
        else
        {
            t++;

            for( ; macrotext[t] != '\n'; t++)
            {
                mstring[msp++] = macrotext[t];
            }
        }
    }// ошибка
    
    return;
 }

 void define_add_to_reprtab()
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
        m_nextch();
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

 void define_add_to_macrotext()
 {
    macrotext[mp++] = 1;

    while(curchar != '\n')
    {
        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }

        if(curchar == '#')
        {
            cur = macro_keywords();
            if(cur == SH_EVAL && curchar == '(')// ошибка
                calculator();
            for(int i = 0; i < csp; i++)
            {
                macrotext[mp++] = cstring[i];
            }
        }

        if (curchar == '\\')
        {
            m_nextch();
            space_end_line();
        }

        macrotext[mp++] = curchar;
        m_nextch();
    }

    macrotext[mp++] = '\n';
 }

 void define_relis()
 {
    if (letter() == 0) 
    { 
        m_error(ident_begins_with_letters);
    }

    define_add_to_reprtab();

    msp = 0;

    if (curchar == '(')
    { 
        m_nextch();
        function_add_to_macrotext();
    }
    else if(curchar != ' ')
    {
        m_error(after_ident_must_be_space);
    }
    else
    {
        space_skip();
        define_add_to_macrotext();   
    }

    return; 
 }
//

//if
 /*int if_count ()
 {
    int iflstring[STRIGSIZE];
    int iflsp;
    int p = 0;
    //oldgifp = gifp;
    //int j = oldgifp;
    while(curchar != '\n')
    {
        ifgstring[p++] = curchar;
        m_nextch();
    }

    while(gifp != p)
    {
        if(ifgstring[gifp] == ')' || ifgstring[gifp] == '(')
        {
            iflstring[iflsp++] = ifgstring[gifp];
            gifp++;

        }
        else if(mdigit(ifgstring[gifp]) || mletter(ifgstring[gifp]) || ifgstring[gifp] == '#'|| ifgstring[gifp] == '-')
        {
            double x,y, type_co;
            x = take_digit();
            type_co = type_co();
            y = take_digit();
            iflstring[iflsp++] = comparison(x, y, type_co);
        }
        else if(ifgstring[gifp] == ' '||ifgstring[gifp] == '\t')
        {
            gifp++;
        }
        else //ошибка
        {
           return 0; 
        }  
    } 
    csp = 0;
    for(csp = 0; csp < iflsp; csp++)
    {
        csring[csp] = iflstring[csp]
    }
    csp++;

    calculator();
    return cstring[0];
 }*/

 int if_check(int type_if)
 {
    int flag = 0;

    if(type_if == SH_IF )
    {
        return 0;//if_count();
    }
    else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
    {
        msp = 0;
        while(letter() || digit())
        {
            mstring[msp++] = curchar;
            m_nextch();
        }
        if( macro_find_ident())
        {
            flag = 1;
        }

        space_end_line();

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

 void if_end()
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
                if_end();
            } 
        }
        else
        {
        m_nextch();
        }
    } 
    m_error(must_be_endif);
 }

 int if_false()
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
                if_end();
            } 
        }
        else
        {
            m_nextch();
        }
    }  
    m_error(must_be_endif);
    return 1;
 }

 void if_true(int type_if)
 {
   while (curchar != EOF )
    {
        preprocess_scan();
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
    
    if_end();
    return;
 }

 void if_relis(int type_if)
 { 
    checkif++;
    int flag = if_check(type_if);// начало (if)
    if(flag)
    {
        if_true(type_if);
        return;
    }
    else
    {
        cur = if_false();
    }

    if (type_if == SH_IF) 
    {
        while (cur == SH_ELIF)
        {
            flag = if_check(type_if);
            space_end_line();
            if(flag)
            {
                if_true(type_if);
                return;
            }
            else
            {
                cur = if_false();
            }

        }
    }
    /*else if (cur == SH_ELIF) ошибка
    {
        printf("Неправильное макрослово\n");
        exit (10);
    }*/

    if (cur == SH_ELSE)// 
    {
        cur = 0;
        if_true(type_if);
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

//основные(preprocess)
 void preprocess_scan()
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
                define_relis();
                m_nextch();
                return;
            }

            else if(cur == SH_IF || cur == SH_IFDEF || cur == SH_IFNDEF)
            {
                if_relis(cur);
                return;
            }
            else if (cur == SH_ELSE || cur == SH_ELIF || cur == SH_ENDIF)
            {
                return;
            }
            else
            {
            m_fprintf(curchar);
            m_nextch();
            //m_error(preproces_words_not_exist); ???
            return;
            }

        case '\'':
                m_fprintf(curchar);
                m_nextch();
                if(curchar == '\\')
                {
                    m_fprintf(curchar);
                    m_nextch(); 
                }
                m_fprintf(curchar);
                m_nextch();
                
                m_fprintf(curchar);
                m_nextch();
                return;

        case '\"':
                m_fprintf(curchar);
                m_nextch();
            while(curchar != '\"' && curchar != EOF)
            {
                if(curchar == '\\')
                {
                    m_fprintf(curchar);
                    m_nextch();  
                }
                m_fprintf(curchar);
                m_nextch();
            }
            m_fprintf(curchar);
            m_nextch();
            return;
        default:
            if(letter() && prep_flag == 1)
            {
                collect_mident();
                define_get_from_macrotext();
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
    mlines[mline = 1] = 1;
    charnum = 1;

    getnext();
    m_nextch();
    while (curchar != EOF )
    {
        preprocess_scan();
    }
    m_conect_lines[mclp++] = mline-1;
 }
//

/*  
 for(int k = 0; k < fsp; k++)
    {
        printf("str[%d] = %d,%c.\n", k, fstring[k], fstring[k]);
    }

    printf("1\n");

            oldcurchar = curchar;
            oldnextchar = nextchar;
            nextch_type = 2;
            ///
            nextch_type = 5;
            curchar = oldcurchar;
            nextchar = oldnextchar;
*/

