struct oper {char code; int prio;};
struct oper operations [6] = { { '*' , 8}, {'/', 8 }, {'+', 7}, {'-',7} };
int digit (char c)
{
	return c<='9' && c>='0' ;
}
int letter (char c)
{
	return c<= 'z' && c>= 'a' ;
}
int opnum (char c)
{
	if (c=='*')
		return 0;
	if (c=='/')
		return 1;
	if (c=='+')
		return 2;
	if (c=='-')
		return 3;
	if (c=='(')
		return 4;
	if (c==')')
		return 5;
	return -1;
}
void main ()
{
	char formula [20], res [15], c;
	int sp=-1, i, rp=-1, p;
	struct oper stack [5];
	getid (formula);
	printid (formula);
	for (i=0; i < 20; i++)
	{ 
		c=formula[i];
		while (c== ' ' && i<19)
		{
			i++;
			c=formula[i]; 
		}
		if ( letter (c) || digit (c) || c==' ' ) 
		{
			res[++rp]=c;
		} 
		else if ((p = opnum (c))< 0)
		{
			print ("bad symbol");
			return;
		}
		else if (c==')')
		{
			while(stack[sp].code != '(' )
			{
				res[++rp]=stack [sp--].code ; 
			}
		sp--; 
		}
		else if (c=='(')
		{
			stack[++sp].prio = 6 ;
			stack [sp].code = '(';

		}
		else
		{
			int prio = operations [p].prio, j; 
			while (sp>=0 && stack [sp].prio >=prio)
				res[++rp]=stack [sp--].code ;
			stack[++sp].prio = prio ;
			stack [sp].code = operations [p].code;
			print (stack);
			printid (res); 
		}

	}
	while (sp>=0 )
		res[++rp]=stack [sp--].code ; 
	print (res); 
	badend : ;
}