float f(float x)
{
	return sin(x);
}

void main()
{
	float a,b,s,s1,l,e=1e-6;
	int i,n;
	getid(a);
	getid(b);
	n=10;
	s=0; s1=0;
	printid(a);
	printid(b);
	do 
	{
		l=(b-a)/n;
		for (i=0;i<n;i++)
		{
			s+=((f(a+i*l) + f(a+(i+1)*l))/2) * l;
		}
		if (abs(s-s1)>l) 
		{ 
			s1=s;
			s=0;
			n=n*2;
		}
		} while(abs(s-s1)>l);
	printid(s);
}
