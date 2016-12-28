
#include "global_vars.h"

extern int  getnext();
extern int  nextch();
extern int  scaner();
extern void error(int e);

int is_row_of_char(int t)
{
	return t > 0 && modetab[t] == MARRAY && modetab[t + 1] == LCHAR;
}

int is_function(int t)
{
	return t > 0 && modetab[t] == MFUNCTION;
}

int is_array(int t)
{
	return t > 0 && modetab[t] == MARRAY;
}

int is_pointer(int t)
{
	return t > 0 && modetab[t] == MPOINT;
}

int is_struct(int t)
{
	return t > 0 && modetab[t] == MSTRUCT;
}

void mustbe(int what, int e)
{
	if (scaner() != what)
		error(e);
}

void totree(int op)
{
	tree[tc++] = op;
}

void totreef(int op)
{
	tree[tc++] = op;
	if (ansttype == LFLOAT && ((op >= PLUSASS && op <= DIVASS) || (op >= PLUSASSAT && op <= DIVASSAT) ||
		(op >= EQEQ && op <= UNMINUS)))
		tree[tc - 1] += 50;
}

int getstatic()
{
    displ += lg;            // смещение от l (полож) или от g (отриц), для структур потом выделим больше места
    if (lg > 0)
        maxdispl = (displ > maxdispl) ? displ : maxdispl;
    else
        maxdisplg = -displ;
    return displ;
}

int toidentab(int f, int type)       // f=0, если не ф-ция, f=1, если метка, f=funcnum, если описание ф-ции,
{                                    // f=-1, если ф-ция-параметр, f>=1000, если это описание типа
//    printf("\n repr %i rtab[repr] %i rtab[repr+1] %i rtab[repr+2] %i\n", repr, reprtab[repr], reprtab[repr+1], reprtab[repr+2]);
	int pred;
	lastid = id;
	if (reprtab[repr + 1] == 0)                  // это может быть только MAIN
	{
		if (wasmain)
			error(more_than_1_main);
		wasmain = id;
	}
	pred = identab[id] = reprtab[repr + 1]; // ссылка на описание с таким же представлением в предыдущем блоке
	if (pred)                                    // pred == 0 только для main, эту ссылку портить нельзя
		reprtab[repr + 1] = id;                  // ссылка на текущее описание с этим представлением (это в reprtab)

	if (f != 1 && pred >= curid)                // один  и тот же идент м.б. переменной и меткой
        
        if (func_def == 3 ? 1 : identab[pred + 1] > 0 ? 1 : func_def == 1 ? 0 : 1)
            error(repeated_decl);  // только определение функции может иметь 2 описания, т.е. иметь предописание

	identab[id + 1] = repr;                     // ссылка на представление
	// дальше тип или ссылка на modetab (для функций и структур)
    
	identab[id + 2] = type;              // тип -1 int, -2 char, -3 float
	if (f == 1)                          // если тип > 0, то это ссылка на modetab
	{
		identab[id + 2] = 0;             // 0, если первым встретился goto, когда встретим метку, поставим 1
		identab[id + 3] = 0;             // при генерации кода когда встретим метку, поставим pc
	}
	else if (f >= 1000)
		identab[id + 3] = f;             // это описание типа, если f > 1000, то f-1000 - это номер иниц проц
	else if (f)
    {
		if (f < 0)
        {
			identab[id + 3] = -(++displ);
            maxdispl = displ;
        }
		else                          // identtab[lastid+3] - номер функции, если < 0, то это функция-параметр
		{
			identab[id + 3] = f;
			if (func_def == 2)
            {
				identab[lastid + 1] *= -1;    //это предописание
                predef[++prdf] = repr;
            }
            else
            {
                int i;
                for (i=0; i<=prdf; i++)
                    if (predef[i] == repr)
                        predef[i] = 0;
            }
		}
    }
	else
        identab[id + 3] = getstatic();
 	id += 4;
	return lastid;
}

void binop(int op)
{
	int rtype = stackoperands[sopnd--];
	int ltype = stackoperands[sopnd];
	if (is_pointer(ltype) || is_pointer(rtype))
		error(operand_is_pointer);
    if ((op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND || op == LSHL || op == LSHR || op == LREM) &&
        (ltype == LFLOAT || rtype == LFLOAT))
        error(int_op_for_float);
	if ((ltype == LINT || ltype == LCHAR) && rtype == LFLOAT)
		totree(WIDEN1);
	if ((rtype == LINT || rtype == LCHAR) && ltype == LFLOAT)
		totree(WIDEN);
	if (ltype == LFLOAT || rtype == LFLOAT)
		ansttype = LFLOAT;
	totreef(op);
	if (op >= EQEQ && op <= LGE)
		ansttype = LINT;
	stackoperands[sopnd] = ansttype;
	//    printf("binop sopnd=%i ltype=%i rtype=%i ansttype=%i\n", sopnd, ltype, rtype, ansttype);
	anst = VAL;
}

void expr(int level);

void exprassn(int);

void toval()        // надо значение положить на стек, например, чтобы передать параметром
{
    if (anst == VAL || anst == NUMBER)
        ;
    else if (is_struct(ansttype))
    {
        if (!inass)
        {
            if (anst == IDENT)
            {
                tc -=2;
                totree(COPY0ST);
                totree(anstdispl);
            }
            else      // тут может быть только ADDR
                totree(COPY1ST);
            totree(modetab[ansttype+1]);
            anst = VAL;
        }
    }
    else
    {
        if (anst == IDENT)
            tree[tc - 2] = TIdenttoval;
        
        if (!(is_array(ansttype) || is_pointer(ansttype)))
            if (anst == ADDR)
                totree(TAddrtoval);
        anst = VAL;
    }
}

void insertwiden()
{
	tc--;
	totree(WIDEN);
	totree(TExprend);
}

void applid()
{
	lastid = reprtab[repr + 1];
	if (lastid == 1)
		error(ident_is_not_declared);
}

int modeeq(int first_mode, int second_mode)
{
    int n, i, flag = 1, mode;
    if (modetab[first_mode] != modetab[second_mode])
        return 0;
    
    mode = modetab[first_mode];
    // определяем, сколько полей надо сравнивать для различных типов записей
    n = mode == MSTRUCT || mode == MFUNCTION ? 2 + modetab[first_mode + 2] : 1;
    
    for (i = 1; i <= n && flag; i++)
        flag = modetab[first_mode + i] == modetab[second_mode + i];
    
    return flag;
}


int check_duplicates()
{
    // проверяет, имеется ли в modetab только что внесенный тип.
    // если да, то возвращает ссылку на старую запись, иначе - на новую.
    
    int old = modetab[startmode];
    
    while (old)
    {
        if (modeeq(startmode + 1, old + 1))
        {
            md = startmode;
            startmode = modetab[startmode];
            return old + 1;
        }
        else
            old = modetab[old];
    }
    return startmode + 1;
}

int newdecl(int type, int elemtype)
{
    modetab[md] = startmode;
    startmode = md++ ;
    modetab[md++] = type;
    modetab[md++] = elemtype;        // ссылка на элемент

    return check_duplicates();
}

void primaryexpr()
{
	if (cur == NUMBER)
	{
		totree(TConst);                              // ansttype задается прямо в сканере
		totree((ansttype == LFLOAT) ? numr : num);   // LINT, LCHAR, FLOAT
		stackoperands[++sopnd] = ansttype;
		//        printf("number sopnd=%i ansttype=%i\n", sopnd, ansttype);
		anst = NUMBER;
	}
	else if (cur == STRING)
	{
		int i = 0;
		ansttype = newdecl(MARRAY, LCHAR); // теперь пишем ansttype в анализаторе, а не в сканере
		totree(TString);
        
		do
            totree(lexstr[i]);
		while (lexstr[i++]);
        
		stackoperands[++sopnd] = ansttype;           // ROWOFCHAR
		anst = VAL;
	}
	else if (cur == IDENT)
	{
		applid();
		totree(TIdent);
		totree(anstdispl = identab[lastid+3]);
		stackoperands[++sopnd] = ansttype = identab[lastid + 2];
		anst = IDENT;
	}
	else if (cur == LEFTBR)
	{
		int oldsp = sp;
		scaner();
		expr(1);
		mustbe(RIGHTBR, wait_rightbr_in_primary);
		while (sp > oldsp)
			binop(stackop[--sp]);
	}
	else if (cur < SLEEP)            // стандартная функция
	{
		int func = cur;
		mustbe(LEFTBR, no_leftbr_in_stand_func);
        if (func == RAND)
        {
            totree(RANDC);
            ansttype = stackoperands[++sopnd] = LFLOAT;
        }
        else
        {
            scaner();
            expr(1);
            if (func == GETDIGSENSOR || func == GETANSENSOR)
            {
                notrobot = 0;
                if (ansttype != LINT)
                    error(param_setmotor_not_int);
                totree(9500 - func);
            }
            else if (func == ABS && ansttype == LINT)
                    totree(ABSIC);
            else
            {
                if (ansttype == LINT || ansttype == LCHAR)
                {
                    totree(WIDEN);
                    ansttype = stackoperands[sopnd] = LFLOAT;
                }
                if (ansttype != LFLOAT)
                    error(bad_param_in_stand_func);
                totree(9500 - func);
                if (func == ROUND)
                    ansttype = stackoperands[sopnd] = LINT;
            }
        }
		mustbe(RIGHTBR, no_rightbr_in_stand_func);
	}
	else
		error(not_primary);
}

void index_check()
{
	if (ansttype != LINT && ansttype != LCHAR)
		error(index_must_be_int);
}

int find_field(int stype)                          // выдает смещение до найденного поля или ошибку
{
    int i, flag = 1, select_displ = 0;
    scaner();
    mustbe(IDENT, after_dot_must_be_ident);
    
    for (i = 0; i < modetab[stype+2]; i+=2)        // тут хранится удвоенное n
    {
        int field_type = modetab[stype+3 + i];
        if (modetab[stype+4 + i] == repr)
        {
            stackoperands[sopnd] = ansttype = field_type;
            flag = 0;
            break;
        }
        else
            if (field_type < 0)
            {
                error(INTERNAL_COMPILER_ERROR);
            }
            select_displ += modetab[field_type] == MSTRUCT ? modetab[field_type + 1] : 1;
            // прибавляем к суммарному смещению длину поля
    }
    if (flag)
        error(no_field);
    return select_displ;
}

void selectend()
{
    while (next == DOT)
        anstdispl += find_field(ansttype);
    
    totree(anstdispl);
    if (is_array(ansttype))
        totree(TAddrtoval);

}

void postexpr()
{
	int lid, leftansttype;
    int was_func = 0;

	primaryexpr();
	lid = lastid;
	leftansttype = ansttype;

	if (next == LEFTBR)                // вызов функции
	{
		int i, j, n, dn, oldinass = inass;
        was_func = 1;
		scaner();
		if (!is_function(leftansttype))
			error(call_not_from_function);

		n = modetab[leftansttype + 2]; // берем количество аргументов функции

		totree(TCall1);
		totree(n);
		j = leftansttype + 3;
		for (i = 0; i<n; i++)          // фактические параметры
		{
			int mdj = modetab[j];      // это вид формального параметра, в ansttype будет вид фактического параметра
			scaner();
			if (is_function(mdj)) // фактическим параметром должна быть функция, в С - это только идентификатор
			{
                if (cur != IDENT)
                    error(act_param_not_ident);
				applid();
				if (identab[lastid + 2] != mdj)
					error(diff_formal_param_type_and_actual);
                dn = identab[lastid+3];
                if (dn < 0)
                {
                    totree(TIdenttoval);
                    totree(-dn);
                }
                else
                {
                    totree(TConst);
                    totree(dn);
                }
                totree(TExprend);
			}
			else
			{
                inass = 0;
				exprassn(0);
                totree(TExprend);
                
                if (mdj > 0 && mdj != ansttype)
                    error(diff_formal_param_type_and_actual);

				if ((mdj == LINT || mdj == LCHAR) && ansttype == LFLOAT)
					error(float_instead_int);
				if (mdj == LFLOAT && (ansttype == LINT || ansttype == LCHAR))
					insertwiden();

				//                 printf("ansttype= %i mdj= %i\n", ansttype, mdj);
				--sopnd;
			}
			if (i < n - 1 && scaner() != COMMA)
				error(no_comma_in_act_params);
			j++;
		}
        inass = oldinass;
		mustbe(RIGHTBR, wrong_number_of_params);
		totree(TCall2);
		totree(lid);
		stackoperands[sopnd] = ansttype = modetab[leftansttype+1];
		anst = VAL;
        if (is_struct(ansttype))
            x -= modetab[ansttype+1] - 1;
	}

    while (next == LEFTSQBR || next == ARROW || next == DOT)
    {
        while (next == LEFTSQBR) // вырезка из массива (возможно, многомерного)
        {
            int elem_type, d;
            if (was_func)
                error(slice_from_func);
            if (modetab[ansttype] != MARRAY)       // вырезка не из массива
                error(slice_not_from_array);

            elem_type = modetab[ansttype + 1];
 
            scaner();
            scaner();

            if (anst == IDENT)               // a[i]
            {
                tree[tc - 2] = TSliceident;
                tree[tc - 1] = anstdispl;
            }
            else                             //a[i][j]
                totree(TSlice);
            
            totree(elem_type);
            expr(0);
            index_check();                   // проверка, что индекс int или char

            mustbe(RIGHTSQBR, no_rightsqbr_in_slice);

            stackoperands[--sopnd] = ansttype = elem_type;
            anst = ADDR;
        }

        while (next == ARROW)  // это выборка поля из указателя на структуру, если больше одной точки подряд - схлопываем в 1 select
        {                      // перед выборкой мог быть вызов функции или вырезка элемента массива
            if (modetab[ansttype] != MPOINT || modetab[modetab[ansttype + 1]] != MSTRUCT)
                error(get_field_not_from_struct_pointer);
            
            if (anst == IDENT)
            {
                tree[tc-2] = TIdenttoval;
                anst = ADDR;
            }
            totree(TSelect);          // может быть, anst уже был ADDR, VAL не может быть адресом структуры

            anstdispl = find_field(ansttype = modetab[ansttype + 1]);
            selectend();
        }
        if (next == DOT)

        {
            if (anst == IDENT)
            {
                int globid = anstdispl < 0 ? -1 : 1;
                while (next == DOT)
                    anstdispl += globid * find_field(ansttype);
                tree[tc-1] = anstdispl;
            }
            else if (anst == ADDR)
            {
                totree(TSelect);
                anstdispl = 0;
                selectend();
            }
            else
                error(get_field_not_from_struct_pointer1);
        }
    }
    
	if (next == INC || next == DEC) // a++, a--
	{
        int op;
		if (ansttype != LINT && ansttype != LCHAR && ansttype != LFLOAT)
			error(wrong_operand);
		if (anst != IDENT && anst != ADDR)
			error(unassignable_inc);
		op = (next == INC) ? POSTINC : POSTDEC;
		if (anst == ADDR)
			op += 4;
		scaner();
		totreef(op);
		if (anst == IDENT)
			totree(identab[lid+3]);
		anst = VAL;
	}
}

void unarexpr()
{
    int op = cur;
	if (cur == LNOT || cur == LOGNOT || cur == LPLUS || cur == LMINUS || cur == LAND || cur == LMULT)
	{
		scaner();
		if (op == LAND || op == LMULT)
		{
			postexpr();
			if (op == LAND)
			{
				if (anst == VAL)
					error(wrong_addr);
                
				if (anst == IDENT)
					tree[tc-2] = TIdenttoaddr;
                
				stackoperands[sopnd] = ansttype = newdecl(MPOINT, ansttype);
                anst = VAL;
			}
			else
			{
				if (!is_pointer(ansttype))
					error(aster_not_for_pointer);
                
                if (anst == IDENT)
                    tree[tc-2] = TIdenttoval;      // *p
                
				stackoperands[sopnd] = ansttype = modetab[ansttype + 1];
                anst = ADDR;
			}
		}
		else
		{
			unarexpr();
			if ((op == LNOT || op == LOGNOT) && ansttype == LFLOAT)
				error(int_op_for_float);
			else if (op == LMINUS)
				totreef(UNMINUS);
			else if (op == LPLUS)
				;
			else
				totree(op);
			anst = VAL;
		}
	}
	else if (cur == INC || cur == DEC)
	{
		scaner();
		postexpr();
		if (anst != IDENT && anst != ADDR)
			error(unassignable_inc);
		if (anst == ADDR)
			op += 4;
		totreef(op);
		if (anst == IDENT)
			totree(identab[lastid+3]);
		anst = VAL;
	}
	else
		postexpr();
    stackoperands[sopnd] = ansttype;
}

void exprinbrkts(int er)
{
	mustbe(LEFTBR, er);
	scaner();
	expr(0);
	mustbe(RIGHTBR, er);
}

int prio(int op)   // возвращает 0, если не операция
{
	return  op == LOGOR ? 1 : op == LOGAND ? 2 : op == LOR ? 3 : op == LEXOR ? 4 : op == LAND ? 5 :
		op == EQEQ ? 6 : op == NOTEQ ? 6 :
		op == LLT ? 7 : op == LGT ? 7 : op == LLE ? 7 : op == LGE ? 7 :
		op == LSHL ? 8 : op == LSHR ? 8 : op == LPLUS ? 9 : op == LMINUS ? 9 :
		op == LMULT ? 10 : op == LDIV ? 10 : op == LREM ? 10 : 0;
}

void subexpr()
{
	int p, oldsp = sp, wasop = 0;
	while ((p = prio(next)))
	{
		wasop = 1;
		toval();
		while (sp > oldsp && stack[sp - 1] >= p)
			binop(stackop[--sp]);

		stack[sp] = p;
		stackop[sp++] = next;
		scaner();
		scaner();
		unarexpr();
	}
	if (wasop)
		toval();
	while (sp > oldsp)
		binop(stackop[--sp]);
}

int opassn()
{
	return
    (next == ASS || next == MULTASS || next == DIVASS || next == REMASS || next == PLUSASS || next == MINUSASS
     || next == SHLASS || next == SHRASS || next == ANDASS || next == EXORASS || next == ORASS)
    ? op = next : 0;
}

void condexpr()
{
	int globtype = 0, adif = 0, r;
	subexpr();                   // logORexpr();
	if (next == QUEST)
	{
		while (next == QUEST)
		{
			int thenref, elseref;
			toval();
			if (ansttype != LINT && ansttype != LCHAR)
				error(float_in_condition);
			totree(TCondexpr);
			thenref = tc++;
			elseref = tc++;
			scaner();
			scaner();
			sopnd--;
			tree[thenref] = tc;
			expr(0);                  // then
			if (!globtype)
				globtype = ansttype;
			sopnd--;
			if (ansttype == LFLOAT)
				globtype = LFLOAT;
			else
			{
				tree[tc] = adif;
				adif = tc++;
			}
			mustbe(COLON, no_colon_in_cond_expr);
			scaner();
			tree[elseref] = tc;
			unarexpr();
			subexpr();   // logORexpr();        else or elif
		}
		toval();
		totree(TExprend);
		if (ansttype == LFLOAT)
			globtype = LFLOAT;
		else
		{
			tree[tc] = adif;
			adif = tc++;
		}

        while (adif != 0)
		{
			r = tree[adif];
			tree[adif] = TExprend;
			tree[adif - 1] = globtype == LFLOAT ? WIDEN : NOP;
			adif = r;
		}

		stackoperands[sopnd] = ansttype = globtype;
	}
	else
	{
		toval();
		stackoperands[sopnd] = ansttype;
	}
}

void exprassnvoid()
{
    if (notcopy)
    {
        int t = tree[tc - 2] < 9000 ? tc - 3 : tc - 2;
        if ((tree[t] >= ASS && tree[t] <= DIVASSAT) || (tree[t] >= POSTINC && tree[t] <= DECAT) ||
            (tree[t] >= PLUSASSR && tree[t] <= DIVASSATR) || (tree[t] >= POSTINCR && tree[t] <= DECATR))
            tree[t] += 200;
        --sopnd;
    }
}

void exprassn(int level)
{
    int leftanst, leftanstdispl, ltype, rtype;
    notcopy = 1;
	unarexpr();

	leftanst = anst;
    leftanstdispl = anstdispl;
    if (opassn())
    {
        int opp = op;
        inass = 1;
        scaner();
        scaner();
        exprassn(level + 1);
        inass = 0;
       
        if (leftanst == VAL)
            error(unassignable);

        if (sopnd <= 0)
        {
            error(INTERNAL_COMPILER_ERROR);
        }

		rtype = stackoperands[sopnd--];      // снимаем типы операндов со стека
		ltype = stackoperands[sopnd];
        
        if (is_array(ltype))                 // присваивать массив в массив в си нельзя
            error(array_assigment);
        
        if (is_struct(ltype))                // присваивание в структуру
        {
			if (ltype != rtype)              // типы должны быть равны
				error(type_missmatch);
            if (opp != ASS)                   // в структуру можно присваивать только с помощью =
                error(wrong_struct_ass);

            if (anst == VAL)
                opp = leftanst == IDENT ? COPY0STASS : COPY1STASS;
            else
                opp = leftanst == IDENT ? anst == IDENT ? COPY00 : COPY01
                                        : anst == IDENT ? COPY10 : COPY11;
            notcopy = 0;
            totree(opp);
            if (leftanst == IDENT)
                totree(leftanstdispl);       // displleft
            if (anst == IDENT)
                totree(anstdispl);           // displright
            totree(modetab[ltype + 1]);      // длина
            anst = leftanst;
            anstdispl = leftanstdispl;
		}
		else // оба операнда базового типа или указатели
		{
            if (is_pointer(ltype) && opp != ASS)        // в указатель можно присваивать только с помощью =
                error(wrong_struct_ass);
			if ((ltype == LINT || ltype == LCHAR) && rtype == LFLOAT)
				error(assmnt_float_to_int);
            
			if ((rtype == LINT || rtype == LCHAR) && ltype == LFLOAT)
            {
				totree(WIDEN);
                ansttype = LFLOAT;
            }
		
            if (leftanst == ADDR)
                opp += 11;
            totreef(opp);
            if (leftanst == IDENT)
                totree(anstdispl = leftanstdispl);
            anst = VAL;
        }
        stackoperands[sopnd] = ansttype = ltype; // тип результата - на стек
	}
	else
		condexpr();    // condexpr учитывает тот факт, что начало выражения в виде unarexpr уже выкушано
}

void expr(int level)
{
	exprassn(level);
	while (next == COMMA)
	{
		exprassnvoid();
		sopnd--;
		scaner();
		scaner();
		exprassn(level);
	}
    if (level == 0)
        totree(TExprend);
}

int arrdef(int t)                 // вызывается при описании массивов и структур из массивов сразу после idorpnt
{
    emptyarrdef = 2;              // == 2, если первый раз, == 1, если были пустые скобки [], == 0, если был [N]
    arrdim = 0;
    if (is_pointer(t))
        error(pnt_before_array);

    while (next == LEFTSQBR)      // это определение массива (может быть многомерным)
    {
        arrdim++;
        scaner();
        if (next == RIGHTSQBR)
        {
            if (emptyarrdef == 2)
                emptyarrdef = 1;
            else if (emptyarrdef == 0)     // int a[]={1,2,3};
                error(empty_init);
        }
        else
        {
            if (emptyarrdef == 1)
                error(empty_init);
            emptyarrdef = 0;
            scaner();
            unarexpr();
            condexpr();
            if (ansttype != LCHAR && ansttype != LINT)
                error(array_size_must_be_int);
            totree(TExprend);
            sopnd--;
        }
        t = newdecl(MARRAY, t); // Меняем тип в identtab (увеличиваем размерность массива)
        mustbe(RIGHTSQBR, wait_right_sq_br);
    }
    return t;
}

int struct_init(int);

int inition(int decl_type)
{
    int all = 1;
    if (decl_type < 0 || is_pointer(decl_type))  // Обработка для базовых типов и указателей
    {
        scaner();
        exprassn(0);
        totree(TExprend);
// съедаем выражение, его значение будет на стеке
        sopnd--;
        if (decl_type < 0)
        {
            if ((decl_type == LINT || decl_type == LCHAR) && ansttype == LFLOAT)
                error(init_int_by_float);
            if (decl_type == LFLOAT && ansttype != LFLOAT)
                insertwiden();
        }
        else if (decl_type != ansttype)
                error(error_in_initialization);
        if (structdispl < 0 )
            structdispl--;
        else
            structdispl++;
    }
    else if (is_struct(decl_type) && next == BEGIN)
            all = struct_init(decl_type);
         else
            error(wrong_init);
    return all;
}

int struct_init(int decl_type)   // сейчас modetab[decl_type] равен MSTRUCT
{
	int next_field = decl_type + 3, num_fields = 0, all = 0;
	mustbe(BEGIN, arr_init_must_start_from_BEGIN);
    
    do
	{
        all += inition(modetab[next_field]);
        next_field += 2;
        num_fields += 2;
        if (num_fields < modetab[decl_type+2])
        {
            if (next == COMMA)        //аргументы идут через запятую, заканчиваются }
                scaner();
            else
                error(no_comma_in_init_list);
        }
    }
    while (num_fields < modetab[decl_type+2]);

    if (next != END)
        error(wait_end);
	scaner();
    return all;
}

int array_init(int decl_type, int N)                    // сейчас modetab[decl_type] равен MARRAY
{
    int elem_type = modetab[decl_type+1], all = 0;
    mustbe(BEGIN, arr_init_must_start_from_BEGIN);
    if (N > 1)
    {
        do
            all += array_init(elem_type, N-1);
        while (scaner() == COMMA);
        if (cur != END)
            error(wait_end);
    }
    else
    {
        do
            all += inition(elem_type);
        while (scaner() == COMMA);
        if (cur != END)
            error(wait_end);
    }
    return all;
}

void decl_id(int decl_type)    // вызывается из block и extdecl, только эта процедура реально отводит память
{                              // если встретятся массивы (прямо или в структурах), их размеры уже будут в стеке
    int oldid = toidentab(0, decl_type),
    elem_len, elem_type,
    all;                       // all - место в дереве, где будет общее количество выражений в инициализации
    
    arrdim = 0;                // arrdim - размерность (0-скаляр), д.б. столько выражений-границ
    elem_type = decl_type;
    
    if (is_struct(decl_type) && next != LEFTSQBR)
    {
        if (lg > 0)
        {
            displ += (modetab[decl_type+1] - 1);
            maxdispl = (displ > maxdispl) ? displ : maxdispl;
        }
        else
        {
            displ -= (modetab[decl_type+1] - 1);
            maxdisplg = -displ;
        }
    }
    
    if (next == LEFTSQBR)                                    // это определение массива (может быть многомерным)
    {
        int adN;
        totree(TDeclarr);
        adN = tc++;
        elem_len = is_struct(decl_type) ? modetab[decl_type+1] : 1;
        decl_type = identab[oldid + 2] = arrdef(decl_type);  // Меняем тип (увеличиваем размерность массива)
        tree[adN] = arrdim;
    }
    totree(TDeclid);
    totree(identab[oldid+3]);
    totree(elem_type);
    totree(arrdim);
    tree[all = tc++] = 0;
    tree[tc++] = is_pointer(decl_type) ? 0 : was_struct_with_arr;
    
    if (next == ASS)
    {
        scaner();

        if (is_array(decl_type))          // инициализация массива
            tree[all] = array_init(decl_type, arrdim);
        else
        {
            structdispl = identab[oldid+3];
            tree[all] = inition(decl_type);
        }
    }
}


void block(int b);
// если b=1, то это просто блок, b=-1 - блок в switch, иначе (b=0) - это блок функции

void statement()
{
	int flagsemicol = 1, oldwasdefault = wasdefault, oldinswitch = inswitch;
	wasdefault = 0;
	scaner();
	if ((cur == LINT || cur == LCHAR || cur == LFLOAT || cur == LVOID || cur == LSTRUCT) && blockflag)
		error(decl_after_strmt);
	if (cur == BEGIN)
	{
		flagsemicol = 0;
		block(1);
	}
	else if (cur == SEMICOLON)
    {
        totree(NOP);
		flagsemicol = 0;
    }
	else if (cur == IDENT && next == COLON)
	{
		int id, i, flag = 1;
		flagsemicol = 0;
		totree(TLabel);
		for (i = 0; flag && i < pgotost - 1; i += 2)
			flag = identab[gotost[i] + 1] != repr;
		if (flag)
		{
			totree(id = toidentab(1, 0));
			gotost[pgotost++] = id;              // это определение метки, если она встретилась до переходов на нее
			gotost[pgotost++] = -line;
		}
		else
		{
			id = gotost[i - 2];
			repr = identab[id + 1];
			if (gotost[i - 1] < 0)
				error(repeated_label);
			totree(id);
		}
		identab[id + 2] = 1;

		scaner();
		statement();
	}
	else
	{
		blockflag = 1;
        
		switch (cur)
		{
		case PRINT:
		{
 					  exprinbrkts(print_without_br);
                      tc--;
                      totree(TPrint);
					  totree(ansttype);
                      totree(TExprend);
					  if (is_pointer(ansttype))
						  error(pointer_in_print);
					  sopnd--;
		}
			break;
		case PRINTID:
		{
						mustbe(LEFTBR, no_leftbr_in_printid);
						mustbe(IDENT, no_ident_in_printid);
						lastid = reprtab[repr + 1];
						if (lastid == 1)
							error(ident_is_not_declared);
						totree(TPrintid);
						totree(lastid);
						mustbe(RIGHTBR, no_rightbr_in_printid);
		}
			break;
		case GETID:
		{
					  mustbe(LEFTBR, no_leftbr_in_printid);
					  mustbe(IDENT, no_ident_in_printid);
					  lastid = reprtab[repr + 1];
					  if (lastid == 1)
						  error(ident_is_not_declared);
					  totree(TGetid);
					  totree(lastid);
					  mustbe(RIGHTBR, no_rightbr_in_printid);
            break;
		}
		case SETMOTOR:
		{
						 notrobot = 0;
						 mustbe(LEFTBR, no_leftbr_in_setmotor);
						 scaner();
						 expr(0);
						 if (ansttype != LINT)
							 error(param_setmotor_not_int);
						 mustbe(COMMA, no_comma_in_setmotor);
						 scaner();
						 expr(0);
						 if (ansttype != LINT)
							 error(param_setmotor_not_int);
						 sopnd -= 2;
						 totree(SETMOTOR);
						 mustbe(RIGHTBR, no_rightbr_in_setmotor);
            break;
		}
		case SLEEP:
		{
					  notrobot = 0;
					  mustbe(LEFTBR, no_leftbr_in_sleep);
					  scaner();
					  expr(0);
					  if (ansttype != LINT)
						  error(param_setmotor_not_int);
					  sopnd--;
					  totree(SLEEP);
					  mustbe(RIGHTBR, no_rightbr_in_sleep);
            break;
		}
		case LBREAK:
		{
					   if (!(inloop || inswitch))
						   error(break_not_in_loop_or_switch);
					   totree(TBreak);
		}
            break;
		case LCASE:
		{
					  if (!inswitch)
						  error(case_or_default_not_in_switch);
					  if (wasdefault)
						  error(case_after_default);
                      totree(TCase);
					  scaner();
					  unarexpr();
					  condexpr();
					  totree(TExprend);
					  if (ansttype == LFLOAT)
						  error(float_in_switch);
					  sopnd--;
					  mustbe(COLON, no_colon_in_case);
                      flagsemicol = 0;
					  statement();
		}
			break;
		case LCONTINUE:
		{
						  if (!inloop)
							  error(continue_not_in_loop);
						  totree(TContinue);
		}
			break;
		case LDEFAULT:
		{
						 if (!inswitch)
							 error(case_or_default_not_in_switch);
						 mustbe(COLON, no_colon_in_case);
						 wasdefault = 1;
                         flagsemicol = 0;
						 totree(TDefault);
 						 statement();
		}
			break;
		case LDO:
		{
					int condref;
                    inloop = 1;
					totree(TDo);
					condref = tc++;
					statement();
					if (next == LWHILE)
					{
						scaner();
						tree[condref] = tc;
						exprinbrkts(cond_must_be_in_brkts);
						sopnd--;
					}
					else
						error(wait_while_in_do_stmt);
					inloop = 0;
		}
			break;
		case LFOR:
		{
					 int fromref, condref, incrref, stmtref;
					 mustbe(LEFTBR, no_leftbr_in_for);
					 inloop = 1;
					 totree(TFor);
					 fromref = tc++;
					 condref = tc++;
					 incrref = tc++;
					 stmtref = tc++;
					 if (scaner() == SEMICOLON)             // init
						 tree[fromref] = 0;
					 else
					 {
						 tree[fromref] = tc;
						 expr(0);
						 exprassnvoid();
						 mustbe(SEMICOLON, no_semicolon_in_for);
					 }
					 if (scaner() == SEMICOLON)             // cond
						 tree[condref] = 0;
					 else
					 {
						 tree[condref] = tc;
						 expr(0);
						 sopnd--;
						 mustbe(SEMICOLON, no_semicolon_in_for);
						 sopnd--;
					 }
					 if (scaner() == RIGHTBR)              // incr
						 tree[incrref] = 0;
					 else
					 {
						 tree[incrref] = tc;
						 expr(0);
						 exprassnvoid();
						 mustbe(RIGHTBR, no_rightbr_in_for);
					 }
					 flagsemicol = 0;
					 tree[stmtref] = tc;
					 statement();
					 inloop = 0;
		}
			break;
		case LGOTO:
		{
					  int i, flag = 1;
					  mustbe(IDENT, no_ident_after_goto);
					  totree(TGoto);
					  for (i = 0; flag && i < pgotost - 1; i += 2)
						  flag = identab[gotost[i] + 1] != repr;
					  if (flag)
					  {                                 // первый раз встретился переход на метку, которой не было, в этом случае
						  totree(-toidentab(1, 0));        // ссылка на identtab, стоящая после TGoto, будет отрицательной
						  gotost[pgotost++] = lastid;
					  }
					  else
					  {
						  int id = gotost[i - 2];
						  if (gotost[id + 1] < 0)          // метка уже была
						  {
							  totree(id);
							  break;
						  }
						  totree(gotost[pgotost++] = id);
					  }
					  gotost[pgotost++] = line;
		}
			break;
		case LIF:
		{
					int thenref, elseref;
					totree(TIf);
					thenref = tc++;
					elseref = tc++;
					flagsemicol = 0;
					exprinbrkts(cond_must_be_in_brkts);
                    tree[thenref] = tc;
					sopnd--;
					statement();
					if (next == LELSE)
					{
						scaner();
						tree[elseref] = tc;
						statement();
					}
					else
						tree[elseref] = 0;
		}
			break;
		case LRETURN:
		{
						int ftype = modetab[functype + 1];
						wasret = 1;
						if (next == SEMICOLON)
						{
							if (ftype != LVOID)
								error(no_ret_in_func);
							totree(TReturn);
						}
						else
						{
							if (ftype == LVOID)
								error(notvoidret_in_void_func);
							totree(TReturnval);
                            totree(ftype > 0 && modetab[ftype] == MSTRUCT ? modetab[ftype+1] : 1);
                            scaner();
							expr(1);
							sopnd--;
							if (ftype == LFLOAT && ansttype == LINT)
								totree(WIDEN);
							else if (ftype != ansttype)
								error(bad_type_in_ret);
                            totree(TExprend);
						}
		}
			break;
		case LSWITCH:
		{
						totree(TSwitch);
						exprinbrkts(cond_must_be_in_brkts);
						if (ansttype != LCHAR && ansttype != LINT)
							error(float_in_switch);
						sopnd--;
						scaner();
						block(-1);
                        flagsemicol = 0;						wasdefault = 0;
						inswitch = oldinswitch;
		}
			break;
		case LWHILE:
		{
					   int doref;
					   inloop = 1;
					   totree(TWhile);
					   doref = tc++;
					   flagsemicol = 0;
					   exprinbrkts(cond_must_be_in_brkts);
					   sopnd--;
					   tree[doref] = tc;
					   statement();
					   inloop = 0;
		}
			break;
		default:
			expr(0);
                
        exprassnvoid();
		}
	}
	if (flagsemicol && scaner() != SEMICOLON)
		error(no_semicolon_after_stmt);
	wasdefault = oldwasdefault;
	inswitch = oldinswitch;
}

int idorpnt(int e, int t)
{
	if (next == LMULT)
	{
		scaner();
        t = newdecl(MPOINT, t);
    }
    mustbe(IDENT, e);
    return t;
}

int gettype();

int struct_decl_list()
{
	int field_count = 0, i, t, elem_type, curdispl = 0, wasarr = 0, tstrbeg;
    int loc_modetab[100], locmd = 3;
    loc_modetab[0] = MSTRUCT;
    tstrbeg = tc;
    totree(TStructbeg);
    tree[tc++] = 0;           // тут будет номер иниц процедуры

    scaner();
    scaner();
    
    do
	{
		t = elem_type = idorpnt(wait_ident_after_semicomma_in_struct, gettype());

        if (next == LEFTSQBR)
        {
            int adN;
            totree(TDeclarr);
            adN = tc++;
            t = arrdef(elem_type);  // Меняем тип (увеличиваем размерность массива)
            tree[adN] = arrdim;
            
            totree(TDeclid);
            totree(curdispl);
            totree(elem_type);
            totree(-arrdim);
            totree(0);              // тут будет all
            totree(was_struct_with_arr); 
            wasarr = 1;
        }
        loc_modetab[locmd++] = t;
        loc_modetab[locmd++] = repr;
        field_count++;
        curdispl += is_struct(t) ? modetab[t+1] : 1;

		if (scaner() != SEMICOLON)
			error(no_semicomma_in_struct);

	}
    while (scaner() != END);
    
    if (wasarr)
    {
        totree(TStructend);
        totree(tstrbeg);
        tree[tstrbeg+1] = was_struct_with_arr = procd++;
        
    }
    else
    {
        tree[tstrbeg] = NOP;
        tree[tstrbeg+1] = NOP;
    }

	loc_modetab[1] = curdispl;                       // тут длина структуры
	loc_modetab[2] = field_count * 2;
    
    modetab[md] = startmode;
    startmode = md++;
	for (i = 0; i < locmd; i++)
		modetab[md++] = loc_modetab[i];

	return check_duplicates();
}

int gettype()
{
    // gettype() выедает тип (кроме верхних массивов и указателей)
    // при этом, если такого типа нет в modetab, тип туда заносится;
    // возвращает отрицательное число(базовый тип), положительное (ссылка на modetab) или 0, если типа не было
    
    was_struct_with_arr = 0;
	if ((type = cur) == LINT || type == LFLOAT || type == LCHAR || type == LVOID)
		return(type);
	else if (type == LSTRUCT)
	{
		if (next == BEGIN)             // struct {
			return(struct_decl_list());
		else if (next == IDENT)
		{
			int l = reprtab[repr + 1];
			scaner();
			if (next == BEGIN)         // struct key {
			{
                // если такое описание уже было, то это ошибка - повторное описание
 				int lid;
				wasstructdef = 1;      // это  определение типа (может быть, без описания переменных)
				toidentab(1000, 0);
 				lid = lastid;
				identab[lid + 2] = struct_decl_list();
                identab[lid + 3] = 1000 + was_struct_with_arr;
                return identab[lid+2];
			}
			else
			{                           // struct key это применение типа
				if (l == 1)
					error(ident_is_not_declared);
                was_struct_with_arr = identab[l+3] - 1000;
				return(identab[l + 2]);
			}
		}
		else
			error(wrong_struct);
	}
    else if (cur == IDENT)
    {
        int l = reprtab[repr+1];
        if (l == 1)
            error(ident_is_not_declared);
        if (identab[l+3] < 1000)
            error(ident_not_type);
        was_struct_with_arr = identab[l+3] - 1000;
        return identab[l+2];
    }
    else
        error(not_decl);
	return 0;
}

void block(int b)
// если b=1, то это просто блок, b=-1 - блок в switch, иначе (b=0) - это блок функции

{
	int oldinswitch = inswitch;
	int notended = 1, i, olddispl, oldlg = lg, firstdecl;
	inswitch = b < 0;
	totree(TBegin);
	if (b)
	{
		olddispl = displ;
		curid = id;
	}
	blockflag = 0;

	while (next == LINT || next == LCHAR || next == LFLOAT || next == LSTRUCT)
	{
        int repeat = 1;
		scaner();
		firstdecl = gettype();
		if (wasstructdef && next == SEMICOLON)
		{
			scaner();
			continue;

		}
		do
		{
			decl_id(idorpnt(after_type_must_be_ident, firstdecl));
			if (next == COMMA)
				scaner();
			else if (next == SEMICOLON)
			{
				scaner();
				repeat = 0;
			}
			else
				error(def_must_end_with_semicomma);
		}
        while (repeat);
	}

	// кончились описания, пошли операторы до }

	do
	{
		if (next == END)
		{
			scaner();
			notended = 0;
		}
		else
			statement();
	}
    while (notended);
                    
	if (b)
	{
		for (i = id - 4; i >= curid; i -= 4)
            reprtab[identab[i + 1] + 1] = identab[i];
		displ = olddispl;
	}
	inswitch = oldinswitch;
	lg = oldlg;
	totree(TEnd);
}


void function_definition()
{
	int fn = identab[lastid + 3], i, pred, oldrepr = repr, ftype, n, fid = lastid;
    int olddispl = displ;
	pgotost = 0;
	functype = identab[lastid + 2];
	ftype = modetab[functype + 1];
	n = modetab[functype + 2];
	wasret = 0;
	displ = 2;
	maxdispl = 3;
	lg = 1;
	if ((pred = identab[lastid]) > 1)            // был прототип
    {
        if (functype != identab[pred + 2])
            error(decl_and_def_have_diff_type);
        identab[pred+3] = fn;
    }
	curid = id;
	for (i = 0; i < n; i++)
	{
		type = modetab[functype + i + 3];
		repr = functions[fn + i + 1];
		if (repr > 0)
        {
			toidentab(0, type);
            if (is_struct(type))
                maxdispl = displ += (modetab[type+1] - 1);
        }
		else
		{
			repr = -repr;
			toidentab(-1, type);
		}
	}
	functions[fn] = tc;
	totree(TFuncdef);
	totree(fid);
	pred = tc++;
	repr = oldrepr;
    
	block(0);
    
//	if (ftype == LVOID && tree[tc - 1] != TReturn)
//	{
		tc--;
		totree(TReturn);
		totree(TEnd);
//	}
	if (ftype != LVOID && !wasret)
		error(no_ret_in_func);
	for (i = id - 4; i >= curid; i -= 4)
        reprtab[identab[i + 1] + 1] = identab[i];

	for (i = 0; i < pgotost - 1; i += 2)
	{
		repr = identab[gotost[i] + 1];
		hash = gotost[i + 1];
		if (hash < 0)
			hash = -hash;
		if (!identab[gotost[i] + 2])
			error(label_not_declared);
	}
	curid = 2;                                 // все функции описываются на одном уровне
	tree[pred] = maxdispl + 1;
	lg = -1;
    displ = olddispl;
}

int func_declarator(int level, int func_d, int firstdecl)
{
	// на 1 уровне это может быть определением функции или предописанием, на остальных уровнях - только декларатором (без идентов)

	int loc_modetab[100], locmd, numpar = 0, ident, maybe_fun, repeat = 1, i, wastype = 0, old;

	loc_modetab[0] = MFUNCTION;
	loc_modetab[1] = firstdecl;
	loc_modetab[2] = 0;
	locmd = 3;

	while (repeat)
	{
		if (cur == LINT || cur == LCHAR || cur == LFLOAT || cur == LSTRUCT)
		{
			maybe_fun = 0;    // м.б. параметр-ф-ция? 0 - ничего не было, 1 - была *, 2 - была [
			ident = 0;        // = 0 - не было идента, 1 - был статический идент, 2 - был идент-параметр-функция
			wastype = 1;
			type = gettype();
            if (next == LMULT)
            {
                scaner();
                type = newdecl(MPOINT, type);
            }
			if (level)
			{
				if (next == IDENT)
				{
					scaner();
					ident = 1;
					functions[funcnum++] = repr;
				}
			}
			else if (next == IDENT)
				error(ident_in_declarator);
            
			if (next == LEFTSQBR)
			{
				maybe_fun = 2;
                
				if (is_pointer(type))
					error(aster_with_row);
                
				while (next == LEFTSQBR)
				{
					scaner();
					mustbe(RIGHTSQBR, wait_right_sq_br);
					type = newdecl(MARRAY, type);
				}
			}
		}
		else if ((type = cur) == LVOID)
		{
			wastype = 1;
			if (next != LEFTBR)
				error(par_type_void_with_nofun);
		}
        
		if (wastype)
		{
			numpar++;
			loc_modetab[locmd++] = type;

			if (next == LEFTBR)
			{
				scaner();
				mustbe(LMULT, wrong_fun_as_param);
				if (next == IDENT)
				{
					if (level)
					{
						scaner();
						if (ident == 0)
							ident = 2;
						else
							error(two_idents_for_1_declarer);
						functions[funcnum++] = -repr;
					}
					else
						error(ident_in_declarator);
				}
				mustbe(RIGHTBR, no_right_br_in_paramfun);
				mustbe(LEFTBR, wrong_fun_as_param);
				scaner();
				if (maybe_fun == 1)
					error(aster_before_func);
				else if (maybe_fun == 2)
					error(array_before_func);

				old = func_def;
				loc_modetab[locmd - 1] = func_declarator(0, 2, type);
				func_def = old;
			}
			if (func_d == 3)
				func_d = ident > 0 ? 1 : 2;
			else if (func_d == 2 && ident > 0)
				error(wait_declarator);
			else if (func_d == 1 && ident == 0)
				error(wait_definition);

			if (scaner() == COMMA)
			{
				scaner();
			}
			else
			if (cur == RIGHTBR)
				repeat = 0;
		}
		else if (cur == RIGHTBR)
		{
			repeat = 0;
			func_d = 0;
		}
		else
			error(wrong_param_list);
	}
	func_def = func_d;
	loc_modetab[2] = numpar;
    
    modetab[md] = startmode;
    startmode = md++;
	for (i = 0; i < numpar + 3; i++)
		modetab[md++] = loc_modetab[i];

	return check_duplicates();
}

void ext_decl()
{
    int i;
	do            // top level описания переменных и функций до конца файла
	{
		int repeat = 1, funrepr, first = 1;
		wasstructdef = 0;
		scaner();
		firstdecl = gettype();
		if (wasstructdef && next == SEMICOLON)
		{                                      // struct point {float x, y;};
			scaner();
			continue;
		}

		func_def = 3;   // func_def = 0 - (), 1 - определение функции, 2 - это предописание, 3 - не знаем или вообще не функция

//		if (firstdecl == 0)
//			firstdecl = LINT;
        
		do            // описываемые объекты через ',' определение функции может быть только одно, никаких ','
		{
            type = firstdecl;
            if (next == LMULT)
            {
                scaner();
                type = newdecl(MPOINT, firstdecl);
            }
            mustbe(IDENT, after_type_must_be_ident);

			if (is_pointer(type) && next == LEFTBR)
				error(aster_before_func);

			if (next == LEFTBR)                // определение или предописание функции
			{
				int oldfuncnum = funcnum++;
				funrepr = repr;
				scaner();
				scaner();
				type = func_declarator(first, 3, firstdecl);  // выкушает все параметры до ) включительно
				if (next == BEGIN)
				{
					if (func_def == 0)
						func_def = 1;
				}
				else if (func_def == 0)
					func_def = 2;
				// теперь я точно знаю, это определение ф-ции или предописание (func_def=1 или 2)
				repr = funrepr;
                
				toidentab(oldfuncnum, type);

				if (next == BEGIN)
				{
					scaner();
					if (func_def == 2)
						error(func_decl_req_params);

					function_definition();
					goto ex;
				}
				else
				{
					if (func_def == 1)
						error(function_has_no_body);
				}
			}
			else
			if (firstdecl == LVOID)
				error(only_functions_may_have_type_VOID);

			// описания идентов-не-функций
            
			if (func_def == 3)
				decl_id(type);
            
			if (next == COMMA)
			{
				scaner();
				first = 0;
			}
			else if (next == SEMICOLON)
			{
				scaner();
				repeat = 0;
			}
			else
				error(def_must_end_with_semicomma);
		}
        while (repeat);
        
	ex:;
	}
    while (next != LEOF);

	if (wasmain == 0)
		error(no_main_in_program);
    for (i=0; i<=prdf; i++)
        if (predef[i])
            error(predef_but_notdef);
}
