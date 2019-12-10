/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "global.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int macrotext[MAXREPRTAB];
int mstring[50];
int macrofunction[MAXREPRTAB];
int functionident[MAXREPRTAB];
int fchange[50];
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
extern void printf_char();
extern void fprintf_char();
extern void m_error();


int mletter(int r);
int mdigit(int r);
int mequal(int str[], int j);

void mend_line();
void m_nextch();
void m_fprintf(int a);

void to_macrotext(char chang[], int oldrepr); //
void macro_reprtab(char chang[]);
void from_macrotext(); // 5
int macro_keywords();  // 12
void relis_define();   // 2

void toreprtab_f();
void to_functionident(); // 4
int scob(int cp);		 // 6
void from_functionident(int r);
void create_change(int r1); // 11
void r_macrofunction();		// 3

// void m_ident();//5
int find_ident();

int check_if(int type_if); // 10
void end_line();		   // 9
void false_if();		   // 8
int m_false();			   // 7
void m_true(int type_if);
void m_if(int type_if);

void macroscan();		// 1,17
void preprocess_file(); // 18

void show_macro()
{
	int i1 = lines[line];
	int str1[50];
	int j = 0;
	int k;
	int flag = 1;
	arg = mlines[m_conect_lines[line]];

	flag_show_macro = 1;
	while (i1 < charnum)
	{
		// printf("\nbe[arg= %i] = %i, so[i1] = %i",arg, before_source[arg],source[i1] );
		if (source[i1] == before_source[arg])
		{
			str1[j++] = before_source[arg];
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

	for (k = 0; k < j; k++)
	{
		printf_char(str1[k]);
	}
	if (flag == 0)
	{
		printf("\n В строке есть макрозамена, строка после макрогенерации:\nline %i)", m_conect_lines[line]);
		for (k = lines[line - 1]; k < charnum; k++)
		{
			printf_char(source[k]);
		}
	}
}

int mletter(int r)
{
	return (r >= 'A' && r <= 'Z') || (r >= 'a' && r <= 'z') || r == '_' || (r >= 0x410 /*А */ && r <= 0x44F /*'я'*/);
}

int mdigit(int r)
{
	return r >= '0' && r <= '9';
}

int mequal(int str[], int j)
{
	int i = 0;
	while (str[i++] == functionident[j++])
	{
		if (str[i] == 0 && functionident[j] == 0)
		{
			return 1;
		}
	}
	return 0;
}

void mend_line()
{
	int j;
	if (flag_show_macro == 0)
	{
		mlines[++mline] = m_charnum;
		mlines[mline + 1] = m_charnum;
		if (kw)
		{
			printf("Line %i) ", mline - 1);
			for (j = mlines[mline - 1]; j < mlines[mline]; j++)
			{
				if (before_source[j] != EOF)
				{
					printf_char(before_source[j]);
				}
			}
		}
	}

	return;
}

void monemore()
{
	if (flag_show_macro == 0)
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
	// printf(" i = %d curcar = %c curcar = %i\n", i, curchar, curchar);
	monemore();

	if (curchar == '/' && nextchar == '/')
	{
		if (i > 13)
		{
			m_fprintf(curchar);
		}
		do
		{
			monemore();
			if (i > 13)
			{
				m_fprintf(curchar);
			}
		} while (curchar != '\n');

		mend_line();
		return;
	}

	if (curchar == '/' && nextchar == '*')
	{
		if (i > 13)
		{
			m_fprintf(curchar);
		}

		monemore();
		if (i > 13)
		{
			m_fprintf(curchar);
		}
		do
		{
			monemore();
			if (i > 13)
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
			{
				mend_line();
			}
		} while (curchar != '*' || nextchar != '/');

		monemore();
		if (i > 13)
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
	if (a == '\n')
	{
		m_conect_lines[mcl++] = mline - 1;
	}
	fprintf_char(output, a);

	return;
}

void to_macrotext(char chang[], int oldrepr)
{
	int i;
	macrotext[mp++] = oldrepr;
	for (i = 0; chang[i] != 0; i++)
	{
		macrotext[mp++] = chang[i];
	}
	macrotext[mp++] = 0;
}

void macro_reprtab(char chang[])
{
	int oldrepr = rp;
	int r;
	int i;


	mlastrp = oldrepr;
	hash = 0;
	rp += 2;

	for (i = 0; i < msp; i++)
	{
		hash += mstring[i];
		reprtab[rp++] = mstring[i];
	}
	msp = 0;
	hash &= 255;
	reprtab[rp++] = 0;
	reprtab[oldrepr] = hashtab[hash];
	reprtab[oldrepr + 1] = mp;

	r = hashtab[hash];
	while (r != 0)
	{
		r = reprtab[r];
	}
	to_macrotext(chang, oldrepr);
	hashtab[hash] = oldrepr;
}

void from_macrotext()
{
	int r;
	msp = 0;

	while (letter() || digit())
	{
		mstring[msp++] = curchar;
		m_nextch(5);
	}

	r = find_ident();
	// printf("r = %d\n", r);

	if (r)
	{
		msp = 0;
		if (reprtab[r + 1] == 2)
		{
			from_functionident(r);
			return;
		}

		r = reprtab[r + 1] + 1;

		for (; macrotext[r] != 0; r++)
		{
			mstring[msp++] = macrotext[r];
		}
	}

	return;
}

int macro_keywords()
{
	int oldrepr = rp;
	int r = 0;

	rp += 2;
	hash = 0;

	do
	{
		hash += curchar;
		reprtab[rp++] = curchar;
		m_nextch(12);
	} while (letter() || digit());

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
	if (r)
	{
		do
		{
			if (equal(r, oldrepr))
			{
				rp = oldrepr;
				return (reprtab[r + 1] < 0) ? reprtab[r + 1] : (repr = r, IDENT);
			}
			else
			{
				r = reprtab[r];
			}
		} while (r);
	}
	return 0;
}

void relis_define()
{
	if (letter())
	{
		msp = 0;
		while (letter() || digit())
		{
			mstring[msp++] = curchar;
			m_nextch(2);
		}

		if (find_ident() != 0)
		{
			m_error(repeat_ident);
		}

		if (curchar == '(')
		{
			// printf("str = %s\n", mstring);
			toreprtab_f();
			m_nextch(2);
			r_macrofunction();
			return;
		}
		else if (curchar != ' ')
		{
			m_error(after_ident_must_be_space);
		}
		else
		{
			int i = 0;
			m_nextch(2);
			char chang[30] = { "\0" };
			while (curchar != '\n')
			{
				chang[i++] = curchar;
				m_nextch(2);
				if (curchar == EOF)
				{
					m_error(not_end_fail_preprocess);
				}
			}
			macro_reprtab(chang);
			return;
		}
	}
	else
	{
		m_error(ident_begins_with_letters);
	}
}

void toreprtab_f()
{
	int i;
	int oldrepr = rp;
	mlastrp = oldrepr;
	// printf("r = %i\n", oldrepr);
	hash = 0;
	rp += 2;
	for (i = 0; i < msp; i++)
	{
		hash += mstring[i];
		reprtab[rp++] = mstring[i];
	}

	hash &= 255;
	reprtab[rp++] = 0;
	reprtab[rp++] = fip;
	reprtab[rp++] = 0;
	reprtab[oldrepr] = hashtab[hash];
	reprtab[oldrepr + 1] = 2;
	hashtab[hash] = oldrepr;
}

void to_functionident()
{
	while (curchar != ')')
	{
		// reportab
		msp = 0;								//   \/
		fip++;									// funcid 5[] -> конец = 13
		if (letter())							//       6[] -> macrofunc
		{										//       7[] -> fcang
												//       8[a]
			while (letter() || digit())			//       9[0]
			{									//      10[] -> fcang
				functionident[fip++] = curchar; //      11[b]
				mstring[msp++] = curchar;		//      12[0]
				m_nextch(4);
			}
			if (find_ident() != 0)
			{
				m_error(repeat_ident);
			}
			functionident[fip++] = 0;
		}
		else
		{
			m_error(functionid_begins_with_letters);
		}
		msp = 0;
		if (curchar == ',' && nextchar == ' ')
		{
			m_nextch(4);
			m_nextch(4);
		}
		else if (curchar != ')')
		{
			m_error(after_functionid_must_be_comma);
		}
	}
	m_nextch(4);
	return;
}

void from_functionident(int r)
{
	int i;
	int kp;
	int cp;
	int r1 = r + 2;
	int str[30];

	for (; reprtab[r1] != 0; r1++)
	{
		;
	}
	r1++;
	r1 = reprtab[r1];
	create_change(r1);

	int finish = functionident[r1];
	int newfi = functionident[r1 + 1];
	int flag = 1;
	msp = 0;
	while (macrofunction[newfi] != '\n')
	{
		if (mletter(macrofunction[newfi]))
		{
			flag = 1;
			for (i = 0; i < 30; i++)
			{
				str[i] = 0;
			}
			i = 0;
			while (mletter(macrofunction[newfi]) || mdigit(macrofunction[newfi]))
			{
				str[i++] = macrofunction[newfi++];
			}
			for (kp = r1 + 2; kp < finish;)
			{
				if (mequal(str, kp + 1))
				{
					for (cp = functionident[kp]; fchange[cp] != '\n'; cp++)
					{
						mstring[msp++] = fchange[cp];
					}
					flag = 0;
					break;
				}
				while (functionident[kp++] != 0)
				{
					;
				}
			}
			if (flag == 1)
			{
				for (i = 0; str[i] != 0; i++)
				{
					mstring[msp++] = str[i];
				}
			}
		}
		else
		{
			mstring[msp++] = macrofunction[newfi];
			newfi++;
		}
	}
}

int scob(int cp)
{
	int i;
	fchange[cp++] = curchar;
	m_nextch(6);
	while (curchar != EOF)
	{
		if (letter())
		{
			from_macrotext();
			for (i = 0; i < msp; i++)
			{
				fchange[cp++] = mstring[i];
			}
		}
		else if (curchar == '(')
		{
			cp = scob(cp);
		}
		else
		{
			fchange[cp++] = curchar;
			m_nextch(6);
			if (curchar != ')')
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

void create_change(int r1)
{
	int i;
	int r = r1 + 2;
	int cp = 1;
	functionident[r] = cp;
	if (curchar == '(')
	{
		m_nextch(11);
		while (curchar != EOF)
		{
			if (letter())
			{
				from_macrotext();
				for (i = 0; i < msp; i++)
				{
					fchange[cp++] = mstring[i];
				}
			}
			else if (curchar == '(')
			{
				cp = scob(cp);
			}
			else if (curchar != ')' && curchar != ',')
			{
				fchange[cp++] = curchar;
				m_nextch(11);
			}
			else
			{
				m_error(not_enough_param);
			}

			if (curchar == ',' || curchar == ')')
			{
				for (; functionident[r] != 0; r++)
				{
					;
				}

				if (r < functionident[r1])
				{
					fchange[cp++] = '\n';
					r++;
				}
				else
				{
					m_error(not_enough_param);
				}

				if (curchar == ',')
				{
					functionident[r] = cp;
					m_nextch(11);
				}
				else
				{
					if (r != functionident[r1])
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

	to_functionident();
	m_nextch(3);
	functionident[olderfip] = fip;

	while (curchar != '\n')
	{
		if (letter())
		{
			from_macrotext();
			for (j = 0; j < msp; j++)
			{
				macrofunction[mfp++] = mstring[j];
			}
			msp = 0;
		}
		else
		{
			macrofunction[mfp++] = curchar;
			m_nextch(3);
		}

		if (curchar == EOF)
		{
			m_error(not_end_fail_preprocess);
		}
	}
	macrofunction[mfp++] = '\n';
	return;
}


/*void m_ident()
{
	msp = 0;

	while(letter() || digit())
	{
		mstring[msp++] = curchar;
		m_nextch(5);
	}
	from_macrotext();

	return;
}*/

int find_ident()
{
	int fpr = rp;
	int i;
	int r;
	hash = 0;
	fpr += 2;
	for (i = 0; i < msp; i++)
	{
		hash += mstring[i];
		reprtab[fpr++] = mstring[i];
	}
	reprtab[fpr++] = 0;
	hash &= 255;
	r = hashtab[hash];
	while (r)
	{
		if (r >= mfirstrp && r <= mlastrp && equal(r, rp))
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

	if (type_if == SH_IF)
	{
		m_error(not_relis_if);
	}
	else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
	{
		msp = 0;
		while (letter() || digit())
		{
			mstring[msp++] = curchar;
			m_nextch(10);
		}
		if (find_ident())
		{
			flag = 1;
		}
		if (type_if == SH_IFDEF)
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
	while (curchar != '\n')
	{
		if (curchar == ' ' || curchar == '\t')
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

void false_if()
{
	int fl_cur;
	while (curchar != EOF)
	{
		if (curchar == '#')
		{
			fl_cur = macro_keywords();
			if (fl_cur == SH_ENDIF)
			{
				checkif--;
				if (checkif == -1)
				{
					m_error(befor_endif);
				}
				return;
			}
			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
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
	while (curchar != EOF)
	{
		if (curchar == '#')
		{
			fl_cur = macro_keywords();
			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}
			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
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
	while (curchar != EOF)
	{
		macroscan();
		if (cur == SH_ELSE || cur == SH_ELIF)
		{
			break;
		}
		if (cur == SH_ENDIF)
		{
			checkif--;
			if (checkif == -1)
			{
				m_error(befor_endif);
			}
			return;
		}
	}

	if (type_if != SH_IF && cur == SH_ELIF)
	{
		m_error(dont_elif);
	}

	false_if();
	return;
}

void m_if(int type_if)
{
	checkif++;
	int flag = check_if(type_if); // начало (if)
	end_line();
	if (flag)
	{
		m_true(type_if);
		return;
	}
	else
	{
		cur = m_false();
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

	if (cur == SH_ELSE) // конец (else)
	{
		cur = 0;
		m_true(type_if);
		return;
	}

	if (cur == SH_ENDIF)
	{
		checkif--;
		if (checkif == -1)
		{
			m_error(befor_endif);
		}
	}
}

void macroscan()
{
	int j;
	switch (curchar)
	{
		case EOF:
			return;

		case '#':
			cur = macro_keywords();
			prep_flag = 1;
			printf("flag");
			if (cur == SH_DEFINE)
			{
				relis_define();
				m_nextch(1);
				return;
			}

			else if (cur == SH_IF || cur == SH_IFDEF || cur == SH_IFNDEF)
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
			m_nextch(171);
			if (curchar == '\\')
			{
				m_fprintf(curchar);
				m_nextch(171);
			}
			m_fprintf(curchar);
			m_nextch(171);

			m_fprintf(curchar);
			m_nextch(171);
			return;

		case '\"':
			m_fprintf(curchar);
			m_nextch(172);
			while (curchar != '\"' && curchar != EOF)
			{
				if (curchar == '\\')
				{
					m_fprintf(curchar);
					m_nextch(172);
				}
				m_fprintf(curchar);
				m_nextch(172);
			}
			m_fprintf(curchar);
			m_nextch(172);
			return;
		default:
			if (letter())
			{
				from_macrotext();
				for (j = 0; j < msp; j++)
				{
					m_fprintf(mstring[j]);
				}
				return;
			}
			else
			{
				m_fprintf(curchar);
				m_nextch(173);
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
	while (curchar != EOF)
	{
		macroscan();
	}
	m_conect_lines[mcl++] = mline - 1;
}
