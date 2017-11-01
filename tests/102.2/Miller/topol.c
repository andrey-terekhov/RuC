int size=1000;
int stack[1000];
int p=-1;
int i,prio,s,t;
char c;
int push(char chto)
{
	if (++p>=size)
		{print("Переполнен"); return(-1);}
		stack[p]=chto;
		return 0;
}


int pop()
	{
	if (p<0) {print("Больше нет");return(-4);}
	switch(stack[p])
	{
	case -2: print('-'); break;
	case  2: print('+'); break;
	case -3: print('/'); break;
	case  3: print('*'); break;
	default: break;
	}			
		p--;
		return 0;
		
	}
void main()
{
print("Press");
print("Write ! in end of line");
getid(c);
while (c!='!')
{
	s=0; prio=0; t=0;
     	if ((c<='9'&&c>='0')||(c>='a'&&c<='z')) print(c);
	if (c=='(')   { s=1;prio=1;}
	if (c==')')   { s=1;prio=-1;}
	if (c=='+')  { s=1;prio=2;}
	if (c=='-')   {s=1; prio=-2;}
	if (c=='/')    { s=1;prio=-3;}
	if (c=='*')   {s=1;prio=3;}
	if (s==1)
		{ if (p==-1) t=1;
		while(t==0) {
			if (abs(stack[p])>=prio) pop();
			else t=1;
			 if (p==-1) t=1;
			 }
		push(prio);
		}	
	getid(c);
}
if (p!=-1) while(p!=-1) pop();	
}	
