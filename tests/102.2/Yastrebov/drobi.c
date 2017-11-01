int okrug (double x)
{
	if (round (x)<=x)
		return round (x);
	else
		return round (x)-1 ; 
}

void main()
{
	int N, D, T=1;
	double n;
	int a0, a1, a2 = 0, i = 0;
	getid(N);
	n = sqrt(N);
	printid(n);
	a0 = okrug ( n);
	D = a0;
	T = N - a0*a0;
	a1 = a0;
	printid(a0);
	while ( a2 != (2*a0) )
	{
		a2=okrug ( (n+D)/T ); 
		D = -D + T*a2;
		T= (N-D*D)/T;
		i++;
		a1=a2;
	}
	printid(i);
}