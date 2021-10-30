struct oper { char code; int prio;};
struct oper operations [4] = {{ '*', 8 }, { '/', 8 }, {'+', 7 }, {'-', 7}};
int digit (char c) 
{
	return '0'<=c && c<='9';
}
int letter( char c)
{
	return 'a'<=c && c<='z';
} 
int opnum( char c)
{
	return c == '*' ? 0 : c == '/' ? 1 : c == '+' ? 2 : c == '-' ? 3 : -1;
}
void main ()
{
	char formula[ 5 ], res[ 10 ],  c;
	int sp = -1, i, rp = -1, p;
	struct oper stack[5];
	getid (formula);
	printid (formula);
	for (i=0; i<5; i++)
	{
		c = formula [i];
		if (letter (c) || digit(c))
			res[++rp] = c;
		else if ( (p = opnum (c)) <0)
		{
			print ( "bad symbol\n");
			goto badend;
		}
		else 
		{
			int prio = operations [p].prio, j;
			while ( sp>=0 && stack[sp].prio >= prio)
				res[++rp] = stack [sp--].code;
			stack[++sp] = { operations[p].code, prio };
		}
	}
	badend:;
    while (sp >= 0)
    {
        res[++rp] = stack[sp--].code;
    }
    printid(res);
}

