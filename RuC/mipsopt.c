//
//  mipsopt.c
//
//  Created by Andrey Terekhov on 21/11/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"
#include <stdlib.h>
extern void tablesandtree();

int t, op, opnd, firststruct = 1;
int tree_size;

/**
 *	Индекс в таблице идентификаторов текущего итератора for
 *	@note мы оптимизируем только циклы с одним итератором
 */
int iterator;

/**
 *	Шаг, с которым увеличивается текущий итератор
 *	@note мы оптимизируем только инкременты с шагом-константой
 */
int step;

/**
 *	Это массив для выражений вырезок индуцированных переменных
 *	Формат записи: индекс начала предыдущей записи, флаг, была ли объявлена переменная, выражение для вырезки
 */
int ind_vars[MAXIDENTAB];

/**
 *	Текущий размер таблицы индуцированных переменных
 */
int ind_vars_counter, ind_vars_start;

/** Проверяет, одинаковые ли выражения для вырезок индуцированных переменных */
int ind_vars_equal(int first_ind_var, int second_ind_var)
{
	int i = 1;

	do
	{
		if (ind_vars[first_ind_var + i] != ind_vars[second_ind_var + i])
			return 0;
		i++;
	} while (ind_vars[first_ind_var + i] != TExprend);
	return 1;
}

int ind_var_add(int *record, int length)
{
	ind_vars[ind_vars_counter] = ind_vars_start;
	ind_vars_start = ind_vars_counter++;

	ind_vars[ind_vars_counter++] = 0; // Еще не была объявлена
	for (int i = 0; i < length; i++)
	{
		ind_vars[ind_vars_counter++] = record[i];
	}

	// Checking duplicates
	int old = ind_vars[ind_vars_start];
	while (old)
	{
		if (ind_vars_equal(ind_vars_start + 1, old + 1))
		{
			ind_vars_counter = ind_vars_start;
			ind_vars_start = ind_vars[ind_vars_start];
			return old + 1;
		}
		else
			old = ind_vars[old];
	}

	return ind_vars_start + 1;
}


int mcopy()
{
//    printf("tc= %i tree[tc]= %i\n", tc, tree[tc]);
    return mtree[mtc++] = tree[tc++];
}

void mtotree(int op)
{
	mtree[mtc++] = op;
}

void insert_tree(int index, int op)
{
	for (int i = tree_size; i >= index; i--)
	{
		tree[i+1] = tree[i];
		// Если есть еще затронутые циклы for, то им нужно сдвинуть ссылки на дерево
		// Костыль, но в будущих итерациях эта функция не понадобится, поэтому пока останется так
		if (tree[i+1] == TFor)
		{
			tree[i+2+check_nested_for]++;
			tree[i+3+check_nested_for]++;
			tree[i+4+check_nested_for]++;
			tree[i+5+check_nested_for]++;
		}
	}

	tree[index] = op;
	tree_size++;
}

void remove_tree(int index)
{
	for (int i = index; i < tree_size; i++)
	{
		// Если есть еще затронутые циклы for, то им нужно сдвинуть ссылки на дерево
		// Костыль, но в будущих итерациях эта функция не понадобится, поэтому пока останется так
		if (tree[i+1] == TFor)
		{
			tree[i+2+check_nested_for]--;
			tree[i+3+check_nested_for]--;
			tree[i+4+check_nested_for]--;
			tree[i+5+check_nested_for]--;
		}
		tree[i] = tree[i+1];
	}

	tree_size--;
}

int munop(int t)  // один операнд, возвращает n+1, где n = числу параметров
{
    switch (t)
    {
        case POSTINC:
        case POSTDEC:
        case INC:
        case DEC:
            
        case POSTINCR:
        case POSTDECR:
        case INCR:
        case DECR:

        case POSTINCAT:
        case POSTDECAT:
        case INCAT:
        case DECAT:

        case POSTINCATR:
        case POSTDECATR:
        case INCATR:
        case DECATR:

        case LNOT:
        case LOGNOT:
        case UNMINUS:
        case UNMINUSR:
         
        case TAddrtoval:
        case TAddrtovalc:
        case TAddrtovalf:
        case WIDEN:
        case WIDEN1:
            
            return 1;

        case TPrint:
		case TGet:
        case TDYNSelect:

            return 2;
            
        default:
            
            return 0;
    }
}

int mbinop(int t)   // два операнда, возвращает n+1, где n = числу параметров
{
    switch (t)
    {
            
        case LOGAND:
        case LOGOR:
            
        case TArassn:
        case TArassni:
        case TArassnc:
            
            op += 1000;
            return 2;
            
        case REMASS:
        case SHLASS:
        case SHRASS:
        case ANDASS:
        case EXORASS:
        case ORASS:
            
        case ASS:
        case PLUSASS:
        case MINUSASS:
        case MULTASS:
        case DIVASS:
            
        case ASSR:
        case PLUSASSR:
        case MINUSASSR:
        case MULTASSR:
        case DIVASSR:

        case REMASSAT:
        case SHLASSAT:
        case SHRASSAT:
        case ANDASSAT:
        case EXORASSAT:
        case ORASSAT:
            
        case ASSAT:
        case PLUSASSAT:
        case MINUSASSAT:
        case MULTASSAT:
        case DIVASSAT:

        case ASSATR:
        case PLUSASSATR:
        case MINUSASSATR:
        case MULTASSATR:
        case DIVASSATR:

        case LREM:
        case LSHL:
        case LSHR:
        case LAND:
        case LEXOR:
        case LOR:
            
        case LPLUS:
        case LMINUS:
        case LMULT:
        case LDIV:
        case EQEQ:
        case NOTEQ:
        case LLT:
        case LGT:
        case LLE:
        case LGE:
            
        case EQEQR:
        case NOTEQR:
        case LLTR:
        case LGTR:
        case LLER:
        case LGER:
        case LPLUSR:
        case LMINUSR:
        case LMULTR:    
        case LDIVR:

            
            op += 1000;
            return 1;
            
        case COPY11:
            op += 1000;
            return 2;
            
        case COPY01:
        case COPY10:
            op += 1000;
            return 3;
            
        case COPY00:
            op += 1000;
            return 4;
            
        default:
            return 0;
    }
}

void mexpr();
int operand();

void mstatement();

void mblock();

void mark_nested_for()
{
	int flag_nested_for_ref = tc;
	int temp_tc = tc + 1;
	while (tree[temp_tc] != TForEnd)
	{
		if (tree[temp_tc] == TFor)
		{
			return;
		}
		temp_tc++;
	}

	if (tree[temp_tc] == TForEnd)
		tree[flag_nested_for_ref] = 1;
}

int satistifies_ind_var_pattern(int t)
{
	// В рамках оптимизации индуцированных переменных мы оптимизируем только
	// инкременты вида i++, i--, ++i, --i, i += const и i -= const
	// Если выражение совпадает с паттерном, то в переменную iterator будет записан id переменной,
	// а в step - шаг увеличения/уменьшения

	if (tree[t] != TIdent)
		return 0;

	// Далее только варианты с tree[t] == TIdent
	if ((tree[t + 2] == POSTINC || tree[t + 2] == INC || tree[t + 2] == POSTDEC || tree[t + 2] == DEC) && tree[t + 3] == TExprend)
	{
		// i++ or ++i
		iterator = tree[t + 1];
		step = (tree[t + 2] == POSTINC || tree[t + 2] == INC) ? 1 : -1;
		return 1;
	}
	else if (tree[t + 2] == TConst && (tree[t + 4] == PLUSASS || tree[t + 4] == MINUSASS) && tree[t + 3] == TExprend)
	{
		// i += const or i -= const
		iterator = tree[t + 1];
		step = tree[t + 3] * (tree[t + 4] == PLUSASS ? 1 : -1);
		return 1;
	}

	return 0;
}

/** Возвращает длину записи вырезки, если подходит, иначе 0 */
int satistifies_ind_slice_pattern(const int slice_start)
{
	int i = slice_start;

	// Для начала нужно проверить, что это вырезка из массива
	if (tree[i] != TSliceident)
		return 0;

	i += 3;
	// Если это вырезка константой a[const]
	if (tree[i] == TConst && tree[i+2] == TExprend)
		i += 3;
	// Если это вырезка идентификатором a[identifier]
	else if (tree[i] == TIdenttoval && tree[i+2] == TExprend)
		// TODO: вставить сюда проверку постоянства tree[i+1] или равенства итератору
		i += 3;
	// Если это вырезка выражением a[identifier ± const]
	else if (tree[i] == TIdenttoval && tree[i+2] == TConst && (tree[i+4] == LPLUS || tree[i+4] == LMINUS) && tree[i+5] == TExprend)
		// TODO: вставить сюда проверку постоянства tree[i+1] или равенства итератору
		i += 6;
	else
		return 0;

	// Здесь мы не проверяем правильность постоения дерева, доверяем парсеру
	while (tree[i] == TSlice)
	{
		// Пропускаем поля с доп. информацией
		i += 2;

		// Если это вырезка константой a[const]
		if (tree[i] == TConst && tree[i+2] == TExprend)
			i += 3;
		// Если это вырезка идентификатором a[identifier]
		else if (tree[i] == TIdenttoval && tree[i+2] == TExprend)
			// TODO: вставить сюда проверку постоянства tree[i+1] или равенства итератору
			i += 3;
		// Если это вырезка выражением a[identifier ± const]
		else if (tree[i] == TIdenttoval && tree[i+2] == TConst && (tree[i+4] == LPLUS || tree[i+4] == LMINUS) && tree[i+5] == TExprend)
			// TODO: вставить сюда проверку постоянства tree[i+1] или равенства итератору
			i += 6;
		else
			return 0;
	}

	// Вообще вроде проверка не нужна, но мне лень сейчас думать
	return i - slice_start;
}

void opt_for_statement()
{
	mcopy();
	int has_nested_for = 1;
	if (check_nested_for)
	{
		mark_nested_for();
		has_nested_for = 1 - tree[tc];
		mcopy();
	}

	int fromref = mcopy();
	int condref = mcopy();
	int incrref = mcopy();
	int tc_stmtref = tree[tree[tc]] == TBegin ? tree[tc] + 1: tree[tc];
	/*int stmtref = */mcopy(); // Эта переменная вообще не нужна

	if (fromref)
		mexpr();
	if (condref)
		mexpr();
	if (incrref)
		mexpr();
  
	if (enable_ind_var && !has_nested_for && incrref && satistifies_ind_var_pattern(incrref))
	{
		// Если инкремент удовлетворяет паттерну, и нет вложенных циклов, то алгоритм оптимизации следующий:
		// (это делается за отдельный проход по телу цикла)
		// Если внутри statement существует вырезка, удовлетворяющая условию, то по текущему значению mtc
		// создаем узлы TIndVar, а вместо вырезок пишем TSliceInd
		// TODO: Также после цикла необходимо добавить IndVar += step * sizeof(element)
		ind_vars_counter = 3; ind_vars_start = 1;
		for (int local_tc = tc; tree[local_tc] != TForEnd; local_tc++)
		{
			int slice_length = satistifies_ind_slice_pattern(local_tc);
			if (slice_length > 0)
			{
				// Добавляем в массив переменных вырезку
				int ind_var_number = ind_var_add(&tree[local_tc], slice_length);

				// Если такой еще не было
				if (ind_vars[ind_var_number] == 0)
				{
					// То добавляем узел TIndVar с этой вырезкой в tree
					insert_tree(tc_stmtref, TIndVar);
					insert_tree(tc_stmtref+1, ind_var_number);
					insert_tree(tc_stmtref+2, step); // Это значение, на которое увеличивается инкремент
					local_tc += 3;
					// Копируем выражение для вырезки
					for (int i = 0; i < slice_length; i++)
					{
						insert_tree(tc_stmtref + 3 + i, tree[local_tc + i]);
						local_tc++;
					}
					// Добавляем в конец узла TIndVar узел TExprend как признак конца
					insert_tree(tc_stmtref + 3 + slice_length, TExprend);
					// Помечаем, что этот узел уже был
					ind_vars[ind_var_number] = 1;
				}

				// Затем заменяем вырезку в оригинальном дереве на TSliceInd
				tree[local_tc] = TSliceInd;
				tree[local_tc + 1] = ind_var_number;
				for (int i = 1; i < slice_length; i++)
					remove_tree(local_tc + 2);
			}
		}
	}

	mstatement();
}

void mstatement()
{
    t = tree[tc];
//  printf("stmt tc-1= %i tree[tc-1]= %i t= %i\n", tc-1, tree[tc-1], t);
    switch (t)
    {
        case TBegin:
            mcopy();
            mblock();
            break;
        case TIf:
        {
            int elseref;
            mcopy();
            elseref = mcopy();
            mexpr();
            mstatement();
            if (elseref)
                mstatement();
            break;
        }
        case TWhile:
        case TSwitch:
        case TCase:
            mcopy();
            mexpr();
            mstatement();
            break;
        case TDefault:
            mcopy();
        case TDo:
            mcopy();
            mstatement();
            mexpr();
            break;
        case TFor:
        	opt_for_statement();
            break;
		case TForEnd:
			mcopy();
			break;
        case TLabel:
            mcopy();
            mcopy();
            mstatement();
            break;
        case TBreak:
        case TContinue:
        case TReturnvoid:
            mcopy();
            break;
        case TReturnval:
            mcopy();
            mcopy();
            mexpr();
            break;
        case TGoto:
        case TPrintid:
        case TGetid:
            mcopy();
            mcopy();
            do
            {
                t = mcopy();
            }while (t != 0);
            break;
            
        case TPrintf:
            mcopy();
            mcopy();   // sumsize
            operand(); // форматная строка
            do
            {
                mexpr();
            }while (tree[tc] != 0);
			mcopy();
            break;
            
        default:
            mexpr();
    }
}

void permute(int n1)
{
    int i, oldopnd1 = tree[tc+1], oldopnd2 = tree[tc+2], oldopnd3 = tree[tc+3];
//    printf("permute sp= %i n1= %i tc= %i op= %i opnd=%i\n", sp, n1, tc, op, opnd);
    for (i=tc+opnd-1; i>n1+opnd-1; i--)
        mtree[i] = mtree[i-opnd];
    mtree[n1] = op;
    if (opnd == 2)
        mtree[n1+1] = oldopnd1;
    if (opnd == 3)
    {
        mtree[n1+1] = oldopnd1;
        mtree[n1+2] = oldopnd2;
    }
    else if (opnd == 4)
    {
        mtree[n1+1] = oldopnd1;
        mtree[n1+2] = oldopnd2;
        mtree[n1+3] = oldopnd3;
    }
    tc  += opnd;
    mtc += opnd;
}

int operand()
{
    int i, n1, flag = 1;
    t = tree[tc];
    if (tree[tc] == NOP)
        mcopy();
    if (tree[tc] == ADLOGOR || tree[tc] == ADLOGAND)
    {
        mcopy();
        mcopy();
    }
    n1 = tc;
    t = tree[tc];
    if (t == TIdent)
    {
        mcopy();
        mcopy();
    }
    else if (t == TString || t == TStringc || t == TStringf)
    {
        int nstr;
        mcopy();
        nstr = mcopy();
        for (i=0; i<nstr; i++)
        {
            mcopy();
            mcopy();
        }
    }
    else if (t == TStringform)
    {
        int nstr;
        mcopy();
        nstr = mcopy();
        for (i=0; i<nstr; i++)
        {
            mcopy();
        }
    }
    else if (t == TSliceident)
    {
        mcopy();
        mcopy();             // displ
		mcopy();         	 // type
		mexpr();         	 // index
        while ((t = tree[tc]) == TSlice)
        {
            mcopy();
			mcopy();         // type
			mexpr();
        }
    }
	else if (t == TSliceInd)
	{
		mcopy();
		mcopy();         	 // ind var number
	}
    else if (t == TCall1)
    {
        int npar;
        mcopy();
        npar = mcopy();
        for (i=0; i<npar; i++)
            mexpr(0);
        mcopy();          // CALL2
        mcopy();
    }
    else if (t == TBeginit)
    {
        int n;
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TStructinit)
    {
        int i, n;
        if (firststruct)
            firststruct = 0;
        else
            flag = 0;
        mcopy();
        n =  mcopy();
        for (i=0; i<n; i++)
            mexpr();
        firststruct = 1;
    }
    else if (t == TIdenttoval || t == TIdenttovalc || t == TIdenttovalf ||
             t == TIdenttoaddr || t == TConst || t == TConstc || t == TConstf)
    {
        mcopy();
        mcopy();
    }
    else if (t == TConstd || t == TIdenttovald || t == TSelect)
    {
        mcopy();
        mcopy();
        mcopy();
    }
    else
        flag = 0;

    return flag ? n1 : 0;
}

void mexpr()
{
    int wasopnd;
    while (1)
    {
		wasopnd = 0;
        while ( (stack[++sp] = operand() ) )
        {
            wasopnd = 1;
//            printf("sp= %i stack[sp]= %i\n", sp, stack[sp]);
        }
        sp--;
        if (tree[tc] == NOP)
            mcopy();
        op = tree[tc];
        if (op == TExprend)
        {
            mcopy();
            if (wasopnd)
                --sp;
            break;
        }
        else if ( (opnd = munop(op)) )
                if (wasopnd)
                    if (op == WIDEN1)
                        permute(stack[sp-1]);
                    else
                        permute(stack[sp]);
                else
                    mcopy(), mcopy();
        else if ( (opnd = mbinop(op)) )
                permute(stack[--sp]);
        
        else if ((op = tree[tc]) == TCondexpr)
        {
//            printf("Cond tc= %i sp= %i\n", tc, sp);
            opnd = 1;
            permute(stack[sp]);
            mexpr();
            mexpr();
        }
        else
            break;
    }
}
    
void init()
{
    int i, n;
    t = tree[tc];
    if (t == TBeginit)
    {
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TStructinit)
    {
        int i, n;
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else
        mexpr();
}
void mblock()
{
    int i, n, all;
    do
    {
		switch (tree[tc])
        {
            case TFuncdef:
            {
                mcopy();     // TFucdef
                mcopy();
                mcopy();
                mcopy();     // TBegin
                mblock();
                break;
            }
                
            case TDeclarr:
            {
                mcopy();     // TDeclarr
                n= mcopy();
                for (i=0; i<n; i++)
                    mexpr();
                break;
            }
            case TDeclid:
            {
                mcopy();    // TDeclid
                mcopy();    // displ
                mcopy();    // type_elem
                mcopy();    // N
                all = mcopy();
                mcopy();    // iniproc
                mcopy();    // usual
                mcopy();    // instruct
                if (all)
                    init();
                break;
            }
			case TIndVar:
			{
				mcopy();
				mcopy();
				mcopy();
				operand();
				break;
			}
            case NOP:
                mcopy();     // NOP
                break;
            case TStructbeg:
            case TStructend:
                mcopy();
                break;
                
            default:
                mstatement();
        }
    }
    while (tree[tc] != TEnd);
    mcopy();
}

void mipsopt()
{
	tree_size = tc;
    sp = -1;
    tc = 0;
    mtc = 0;
	ind_vars[1] = 0;
	ind_vars[2] = 0;
	ind_vars[3] = TExprend;
    mblock();
}
