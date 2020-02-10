#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "global_vars.h"

#define STRIGSIZE 100
#define DIP 15


#define CANGEEND -6
#define MACROEND -5
#define MACROUNDEF -3
#define MACROCANGE -4
#define WHILEBEGIN -2
#define MACROFUNCTION 0
#define MACRODEF 1

#define SOURSTYPE 1
#define MTYPE 2
#define CTYPE 3
#define IFTYPE 4
#define WHILETYPE 5
#define FTYPE 6
#define TEXTTYPE 10

int macrotext[MAXREPRTAB];
int mp = 1;
int oldmp = 1;

int mstring[STRIGSIZE];
int msp = 0;

//int fstring[STRIGSIZE];
//int fsp = 0;
//int oldfsp = 0;

int fchange[STRIGSIZE*3];
int cp = 0;

int localstack[STRIGSIZE];
int lsp = 0;

int cstring[STRIGSIZE];
int csp = 0;

int ifstring[STRIGSIZE];
int ifsp = 0;

int wstring[STRIGSIZE*5];
int wsp;

int mfirstrp = -1; // начало и конец макрослов в reprtab
int mlastrp = -1;

int mclp = 1;
int checkif = 0;
int flagint = 1;

int nextp = 0;
int nextch_type = 0;
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

void a_erorr(int i)
{
    printf("не реализованная ошибка № %d\n", i);
      //for(int k = 0; k < mp; k++)
            {
            //    printf("macrotext[%d] = |%d|%c|\n", k, macrotext[k], macrotext[k]);
            }
        exit(1);
}

/*void show_macro()
{
    int i, j = mlines[m_conect_lines[line]];
    int flag = 1;
    printf("line %i) ", j);
    for(i = lines[line]; i < charnum; i++)
    {
        printf_char(source[i]);
        if(flag && source[i] == before_source[j])
        {
            j++;
        }
        else
        {
            flag = 0;
        }
    }

    if(flag == 0)
    {
      printf("\n В строке есть макрозамена, строка до макрогенерации:\nline %i)",m_conect_lines[line]);
        for (j = mlines[m_conect_lines[line]]; j < mlines[m_conect_lines[line] + 1]; j++)
        {
        printf_char(before_source[j]);
        }                  
    }
}*/

//простые (m)

 void m_change_nextch_type(int type, int p)
 {
    
    if (type != SOURSTYPE)
    {
        oldcurchar[dipp] = curchar;
        oldnextchar[dipp] = nextchar;
        oldnextch_type[dipp] = nextch_type;
        oldnextp[dipp] = nextp;
        nextp = p;
        dipp++;
    }
    else
    {
        dipp = 0;
    }
    
    //printf("nextch_type\n");
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
    //printf("oldnextch_type = %d\n",nextch_type);
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

 int m_equal()
 {
    int i = 0;
    int n = 1;  
    int j = 0;
    while (j < csp)
    {
        while (mstring[i++] == cstring[j++]) 
        {
           if (mstring[i] == MACROEND && cstring[j] == 0)
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

 int mf_equal(int i)
 {
    int j = 0;
    ++i;
    ++i;
    while (reprtab[i++] == mstring[j++])
    {
        if (reprtab[i] == 0 && mstring[j] == MACROEND)
            return 1;
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

    printf("Line %i) ", mline - 1);

    for ( j=mlines[mline - 1]; j<mlines[mline]; j++)
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
        m_conect_lines[mclp++] = mline - 1;
    }
    fprintf_char(output, a);
    //printf_char(a);
    //printf(" %d ", a);
    //printf(" t = %d n = %d\n", nextch_type, nextp);
    return;
 }

 void m_fprintf_com()
 {
    if(nextch_type == 17)
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
            if (curchar == EOF)
            {
                return;
            }
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
 
 void m_nextch_cange()
 {
    m_nextch();
    //printf("2 lsp = %d curchar = %d l = %d\n",  lsp, curchar, curchar + lsp);
    m_change_nextch_type(FTYPE, localstack[curchar + lsp]);
 }

 void m_nextch()
 {
    if(nextch_type != 0 && nextch_type <= TEXTTYPE)
    {
        if(nextch_type == SOURSTYPE)
        {
            curchar = before_source[nextp++];
            nextchar = before_source[nextp];
        }
        else if(nextch_type == MTYPE && nextp < msp)
        {
            curchar = mstring[nextp++];
            nextchar = mstring[nextp];
        }
        else if(nextch_type == CTYPE && nextp < csp)
        {
            curchar = cstring[nextp++];
            nextchar = cstring[nextp];
        }
        else if(nextch_type == IFTYPE && nextp < ifsp)
        {
            curchar = ifstring[nextp++];
            nextchar = ifstring[nextp];
        }
        else if(nextch_type == WHILETYPE && nextp < wsp)
        {
            curchar = wstring[nextp++];
            nextchar = wstring[nextp];
        }
        else if(nextch_type == TEXTTYPE && nextp < mp)
        {
            curchar = macrotext[nextp++];
            nextchar = macrotext[nextp];
            //printf(" i = %d curcar = %c curcar = %i n = %d\n", nextch_type, curchar, curchar, nextp);

            if (curchar == MACROCANGE)
                m_nextch_cange();
            else if(curchar == MACROEND)
                m_old_nextch_type();
        }
        else if(nextch_type == FTYPE)
        {
            curchar = fchange[nextp++];
            nextchar = fchange[nextp];
            if (curchar == CANGEEND)
            {
                m_old_nextch_type();
                m_nextch();
            }
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
    //printf(" i = %d curcar = %c curcar = %i n = %d\n", nextch_type, curchar, curchar, nextp);

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

 void space_skip_str()
 {
    int c = curchar;
    m_fprintf(curchar);
    m_nextch();

    while(curchar != c && curchar != EOF)
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
 }
//

 int collect_mident()
 {
    int r;
    int hash = 0;
    msp = 0;

    while(letter() || digit())
    {
        mstring[msp++] = curchar;
        hash += curchar;
        m_nextch();
    }
    mstring[msp] = MACROEND;
    hash &= 255;
    r = hashtab[hash];
    while(r)
    {
        if(r >= mfirstrp && r<=mlastrp && mf_equal(r) )
        {
           return (macrotext[reprtab[r+1]] != MACROUNDEF) ? r : 0;
        }
        r = reprtab[r];        
    }
    return 0;
 }

// macro

 int macro_keywords() 
 {
    int oldrepr = rp;
    int r = 0;
    int n = 0;
    rp+=2;
    hash = 0;               
    do
    {                 
        hash += curchar;
        reprtab[rp++] = curchar;
        n++;
        m_nextch();
    }
    while(letter() || digit());

    if (curchar != '\n' && curchar != ' ' && curchar != '\t' && curchar != '(')
    {
        m_error(after_ident_must_be_space);
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
            reprtab[rp] = n;
            return(reprtab[r+1] < 0) ? reprtab[r+1] : 0;
        }
        else
        r = reprtab[r];
        }
        while(r);
    }
    rp = oldrepr;
    reprtab[rp] = n;
    return 0;
 }

//

//eval
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
    if(c == '|' || c == '&' || c == '='||c == '!')
    {
        if((nextchar == c && c != '!') || (c == '!'  && nextchar == '='))
        {
            m_nextch();
            m_nextch();
            return c;
        }
        else
        {
            return 0;
        }
    }
    else if(c == '>' && nextchar == '=')
    {
        m_nextch();
        m_nextch();
        return 'b';   
    }
    else if(c == '>' && nextchar == '=')
    {
        m_nextch();
        m_nextch();
        return 's';   
    }
    else if (c == '>' || c == '<' || c == '+'||c == '-'||c == '*'||c == '/'||c == '%'||c == '(')
    {
        m_nextch();
        return c;
    }
    else
        return 0;
    
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
        case '<':
        case '>':
        case 's':
        case 'b':
        case '=':
        case '!':
            return 3;
        case '+':
        case '-':
            return 4;
        case '*':
        case '/':
        case '%':
            return 5;
        default:
            return 0;
    }
 }

 double relis_opiration(double x, double y, int r,int int_flag)
 {
    switch(r)
    {
        case '<':
            return x < y;
        case '>':
            return x > y;
        case 's':
            return x <= y;
        case 'b':
            return x >= y;
        case '=':
            return x == y;
        case '!':
            return x != y;
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
                   return (int)x / (int)y;
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
            return 0;
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

 void calculator(int if_flag)
 {
    int i = 0; 
    int op = 0;
    int c;
    double stack[10];
    int int_flag = 1;
    int operation[10];
    int opration_flag = 0;
    if(!if_flag)
    {
        operation[op++] = '(';
        m_nextch();
    }
    while(curchar != '\n')
    {
        space_skip();

        if((digit() || curchar == '-' && m_digit(nextchar)) && !opration_flag)
        {
            opration_flag = 1;
            stack[i++] = get_digit();
            int_flag = flagint && int_flag;
             //for(int k = 0; k < i; k++)
            {
             //   printf(" stack[%d] = %lf.\n", k, stack[k]);
            }
        } 
        else if (letter())
        {
            //for(int k = 0; k < mp; k++)
            {
            //    printf("macrotext[%d] = %d,%c.\n", k, macrotext[k], macrotext[k]);
            } 
            int r = collect_mident();
                if(r)
                    define_get_from_macrotext(r);
                else
                    a_erorr(-2);
            //printf("4 i = %d curcar = %c curcar = %i n = %d ms = %d\n", nextch_type, curchar, curchar, nextp, );
        }
        else if (curchar == '#' && if_flag)
        {
            cur = macro_keywords();
            if(cur == SH_EVAL && curchar == '(')
                calculator(0);
            else
                a_erorr(1);
            m_change_nextch_type(CTYPE, 0);
        }
        else if((opration_flag|| curchar == '(') && (c = check_opiration()))
        { 
            int n = get_prior(c); 
            opration_flag = 0;
            if (n != 0 && ( if_flag && n > 3 || !if_flag && n <=3))
                a_erorr(2);
            while(op != 0 && n != 0 && get_prior(operation[op - 1]) >= n)
            {
                stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag);
                op--;
                i--;
            }
            operation[op++] = c;  
            //printf("op = %d i = %d\n",op, i); 
        }
        else if(curchar == ')')   
        { 
            while(operation[op - 1] != '(')
            {
                //printf("op = %d i = %d\n",op, i);
                if(i < 2 ||op == 0)
                    a_erorr(3);
                stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag);
                op--;
                i--;
            }
            op--;
            m_nextch();
            if(op == 0 && !if_flag)
            {
                double_to_string(stack[0], int_flag); 
                return;
            }
        } 
        else if (curchar != '\n')exit(1);
    }

    if (if_flag)
    {
        csp = 0;
        while (op > 0)
        {
            if(i < 2)
                a_erorr(4);
            stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag);
            op--;
            i--;
        }
        if(stack[0] == 0)
            cstring[0] = 0;
        else
            cstring[0] = 1;      
    }
    else
        a_erorr(5);
 }
//

//define c параметрами (function)
 void function_scob_collect(int t, int num)
 {
    int i;
    while(curchar != EOF)
    {
        if(letter())
        {
            int r = collect_mident();
            if(r)
            {
                int oldcp1 = cp;
                int oldlsp = lsp;
                int locfchange[STRIGSIZE];
                int lcp = 0;
                int ldip;
                lsp += num;
                define_get_from_macrotext(r);
                ldip = dipp;
                if(nextch_type == FTYPE)
                    ldip--;
                while(dipp >= ldip)// 1 переход потому что есть префиксная замена 
                {
                    locfchange[lcp++] = curchar;
                    m_nextch();
                }
                lsp = oldlsp;
                cp = oldcp1;
                for (i = 0; i < lcp; i++)
                {
                    fchange[cp++] = locfchange[i];  
                } 
            }
            else
                for(i = 0; i < msp; i++)
                 {
                    fchange[cp++] = mstring[i];  
                } 
                    
        }
        else if(curchar == '(')
        {
            fchange[cp++] = curchar;
            m_nextch();
            function_scob_collect(0, num);
        }
        else if(curchar == ')'|| (t == 1 && curchar == ','))
        {
            if(t == 0)
            {
                fchange[cp++] = curchar;
                m_nextch();
            }
            return;
        }
        else if(curchar == '#')
        {
            if(macro_keywords() == SH_EVAL && curchar == '(')
            {
                calculator(0);
                for(i = 0; i < csp; i++)
                    fchange[cp++] = cstring[i];
            }
            else
            {
                for(i = 0; i < reprtab[rp]; i++)
                    fchange[cp++]= reprtab[rp + 2 + i];
            }
        }
        else
        {
            fchange[cp++] = curchar;
            m_nextch();
        }
    }
    m_error(scob_not_clous);
    return;
 }

 void function_stack_create(int n)
 {
    int i;
    int num = 0;
    m_nextch();
    //printf("function_stack_create n = %d\n", n);
    localstack[num + lsp] = cp;        

    if(curchar == ')')
        m_error(stalpe);
    
    while(curchar != ')')
    {
        function_scob_collect(1, num);
        fchange[cp++] = CANGEEND;
 
        if (curchar == ',')
        {  
            num++;
            localstack[num + lsp] = cp;
            if(num > n)
                m_error(not_enough_param);
            m_nextch();
            if(curchar == ' ')
                m_nextch();
        }    
        else if (curchar == ')')
        {
            if(num != n)
                m_error(not_enough_param2);    
            m_nextch();
            
            cp = localstack[lsp];
            return;
        }       
    }
    m_error(scob_not_clous);    
 }

 void funktionleter(int flag_macro)
 {
    int n = 0;
    int i = 0;
    msp = 0;
    int r = collect_mident();
    //printf("funktionleter\n");
    if((n = m_equal()) != 0)
    {
        macrotext[mp++] = MACROCANGE;
        macrotext[mp++] = n - 1;
    } 
    else if(!flag_macro && r)
    {
        define_get_from_macrotext(r);
    }
    else
        for(i = 0; i < msp; i++)
            macrotext[mp++] = mstring[i];
 
 }

 int to_functionident()
 {
    int num = 0;
    csp = 0;
    //printf("to_functionident\n");
    while(curchar != ')')
    { 
        msp = 0; 
        if (letter()) 
        {
            while(letter() || digit())
            {
                cstring[csp++] = curchar;
                m_nextch();
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
    //printf("-to_functionident = %d\n", num);
    m_nextch();
    return num; 
 }

 void function_add_to_macrotext()
 {
    int j;
    int flag_macro = 0;
    int empty = 0;
    //printf("function_add_to_macrotext\n");

    if(cur == SH_MACRO)
        flag_macro = 1;
   
    macrotext[mp++] = MACROFUNCTION;

    if(curchar == ')')
    {
       macrotext[mp++] = -1;
       empty = 1; 
       m_nextch();
    }
    else
    {
        macrotext[mp++] = to_functionident();
    }
    space_skip();
    
    while(curchar != '\n' || flag_macro && curchar != EOF)
    {

        if (letter() && !empty)
        {    
            funktionleter(flag_macro);
        }
        else if(curchar == '#')
        {
            cur = macro_keywords();
            if(!flag_macro && cur == SH_EVAL && curchar == '(')
            {
                calculator(0);
                for(j = 0; j < csp; j++)
                    macrotext[mp++] = cstring[j];
            }
            else if(flag_macro && cur == SH_ENDM)
            {
                m_nextch();
                macrotext[mp++] = MACROEND;
                return;
            }
            else
            {
                cur = 0;
                for(j = 0; j < reprtab[rp]; j++)
                    macrotext[mp++] = reprtab[rp + 2 + j];
            }
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
    macrotext[mp++] = MACROEND;
    return;
 }
//

//define
 void define_get_from_macrotext(int r)
 {
    int t = reprtab[r + 1];
    //printf("from_macrotext r = %d\n", macrotext[t]);
    if(r)
    {
        msp = 0;
        if (macrotext[t] == MACROFUNCTION)
            if(macrotext[++t] > -1)
                function_stack_create(macrotext[t]);
    //printf("--from_macrotext r = %d\n", t + 1);     
        m_change_nextch_type(TEXTTYPE, t + 1);
    }
    else
        a_erorr(-1);
    return;
 }

 int define_add_to_reprtab()
 {
    int i;
    int r;
    int oldrepr = rp;
    //printf("define_add_to_reprtab\n");

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
            if(macrotext[reprtab[r+1]] == MACROUNDEF)
            {
                rp = oldrepr;
                return r;
            }
            else
            {
                a_erorr(10);
            }
        }
        r = reprtab[r];        
    }
    
    reprtab[oldrepr] = hashtab[hash];
    reprtab[oldrepr + 1] = mp;
    hashtab[hash] = oldrepr;
    return 0;
 }

 void define_add_to_macrotext(int r)
 {
    int j, lmp = mp;
    //printf("define_add_to_macrotext =%d\n", r);

    macrotext[mp++] = MACRODEF;
    while(curchar != '\n')
    {
        if(curchar == EOF)
        {
            m_error(not_end_fail_preprocess);
        }
        else if(curchar == '#')
        {
            cur = macro_keywords();
            if(cur == SH_EVAL && curchar == '(')
            {
                calculator(0);
                for(j = 0; j < csp; j++)
                {
                    macrotext[mp++] = cstring[j];
                }
            }
            else
            {
                for(j = 0; j < reprtab[rp]; j++)
                {
                    macrotext[mp++] = reprtab[rp + 2 + j];
                }
            }
        }
        else if (curchar == '\\')
        {
            m_nextch();
            space_end_line();
        }
        else if (letter())
        {
            int k = collect_mident();
            if(k)
                define_get_from_macrotext(k);
            else
                for(j = 0; j < msp; j++)
                    macrotext[mp++] = mstring[j];
        }
        else
        {
            macrotext[mp++] = curchar;
            m_nextch();
        }
    }

    while(macrotext[mp-1] == ' ' || macrotext[mp-1] == '\t')
    {
        macrotext[mp-1] == MACROEND;
        mp--;
    }
    macrotext[mp++] = MACROEND;
    reprtab[r+1] = lmp;

    //for(int k = 0; k < mp; k++)
    {
       // printf("1macrotext[%d] = %d,%c.\n", k, macrotext[k], macrotext[k]);
    }
 }

 void define_relis()
 {
     
    int r;
    //printf("define_relis mp = %d\n",mp);
    if (!letter()) 
    { 
        m_error(ident_begins_with_letters);
    }

    r = define_add_to_reprtab();

    msp = 0;

    if (curchar == '(' && !r)
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
        define_add_to_macrotext(r);   
    }
    m_nextch();

    return; 
 }
//

//if
 int if_check(int type_if)
 {
    int flag = 0;

    if(type_if == SH_IF )
    {
       calculator(1);
       return cstring[0];
    }
    else
    {
        msp = 0;
        if(collect_mident())
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
 }

 void if_end()
 {
    int fl_cur;
   while(curchar != EOF)
    {
        if (curchar == '#')
        {
            fl_cur = macro_keywords();
            m_nextch();
            if(fl_cur == SH_ENDIF)
            {
                checkif--;
                if(checkif < 0)
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
            m_nextch();
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
            if(checkif < 0)
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

 void if_relis()
 { 
    int type_if = cur;
    int flag = if_check(type_if);// начало (if)
    checkif++;
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
    else if (cur == SH_ELIF)
    {
        a_erorr(7);
    }

    if (cur == SH_ELSE)// 
    {
        cur = 0;
        if_true(type_if);
        return;
    }

    if(cur == SH_ENDIF)
    {
        checkif--;
        if(checkif < 0)
        {
            m_error(befor_endif);
        }
    }
 }
//

//while
 void while_collect()
 {
    int oldwsp = wsp;
    wstring[wsp++] = WHILEBEGIN;
    wstring[wsp++] = ifsp;
    wsp++;

    while (curchar != '\n')
    {
        ifstring[ifsp++] = curchar;
        m_nextch();
    }
    ifstring[ifsp++] ='\n';
    m_nextch();
    while (curchar != EOF)
    {
        if(curchar == '#')
        {
            cur = macro_keywords();
            if(cur == SH_WHILE)
            {
                while_collect();
            }
            else if(cur == SH_ENDW)
            {
                wstring[oldwsp+2] = wsp;
                cur = 0;
                return;
            }
            else
            {
                int i = 0;
                for(i = 0; i < reprtab[rp]; i++)
                    wstring[wsp++] = reprtab[rp + 2 + i];
            }    
        }
        wstring[wsp++] = curchar;
        m_nextch();
    }  
    a_erorr(8); 
 }

 void while_relis()
 { 
    int oldernextp = nextp;
    int end = wstring[oldernextp+2];
    cur = 0; 
    while(wstring[oldernextp] == WHILEBEGIN)
    {
        m_nextch();
        m_change_nextch_type(IFTYPE, wstring[nextp]);
        calculator(1);
        m_old_nextch_type();
        if(cstring[0] == 0)
        {
            nextp = end;
            m_nextch();
            return;
        }
        m_nextch();
        m_nextch();
        m_nextch();
        space_skip();
        while (nextp != end || nextch_type != WHILETYPE)
        {
            if(curchar == WHILEBEGIN)
            {
                nextp--;
                while_relis();
            }
            else if(curchar == EOF)
                a_erorr(12);
            else
            {
                preprocess_scan();
            }    
        }
        nextp = oldernextp;
    }
 }
//

//основные(preprocess)
 void preprocess_words()
 { 
    int j;
    if(curchar != '(')
        m_nextch();
    switch(cur)
    {
        case SH_DEFINE:
        case SH_MACRO:
            define_relis();
            return;
        case SH_UNDEF:
        {
            int k;   
            macrotext[reprtab[(k = collect_mident())+1]] = MACROUNDEF;
            return;
        }
        case SH_IF:
        case SH_IFDEF:
        case SH_IFNDEF:
            if_relis();
            return;
        case SH_SET:
            j = collect_mident();
            m_nextch();
            define_add_to_macrotext(j);
            return;
        case SH_ELSE:
        case SH_ELIF: 
        case SH_ENDIF:
            return;
        case SH_EVAL:
            if(curchar == '(')
                calculator(0);
            else
                a_erorr(1);
            m_change_nextch_type(CTYPE, 0);
            return;
        case SH_WHILE:
            wsp = 0;
            ifsp = 0;
            while_collect();
            m_change_nextch_type(WHILETYPE, 0);
            nextp = 0;
            while_relis();
            return;
        default:
            m_nextch();
            for(j = 0; j < reprtab[rp]; j++)
                m_fprintf(reprtab[rp + 2 + j]);
            return;
    }
 }

 void preprocess_scan()
 {  
    int i;
    switch(curchar)
    {
        case EOF:
            return ;

        case '#':
            cur = macro_keywords ();
            prep_flag = 1;
            preprocess_words();
            return;    
        case '\'':
        case '\"':
            space_skip_str();
            return;
        default:
            if(letter() && prep_flag == 1)
            {
                int r = collect_mident();
                if(r)
                    define_get_from_macrotext(r);
                else
                    for(i = 0; i < msp; i++)
                        m_fprintf(mstring[i]);
            }
            else
            {
                m_fprintf(curchar);
                m_nextch();
            }

            return;
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

    m_conect_lines[mclp++] = mline - 1;
 }
//

/*  
 for(int k = 0; k < fsp; k++)
    {
        printf("str[%d] = %d,%c.\n", k, fstring[k], fstring[k]);
    }

    printf("1\n");

 for(int k = 0; k < mp; k++)
            {
                printf("macrotext[%d] = %d,%c.\n", k, macrotext[k], macrotext[k]);
            }
            for(int k = 0; k < cp; k++)
            {
                printf("localstack[%d] = %d,%c.\n", k, localstack[k], localstack[k]);
            }
             for(int k = 0; k < cp; k++)
            {
                printf(" fchange[%d] = %d,%c.\n", k, fchange[k], fchange[k]);
            }
        
*/

