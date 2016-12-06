int f(int x)
{	
	if(x>0) 
		return x*f(x-1);
	if(x==0)
		return 1;	
}


void main()
{	
	int n;
	int fakt;
	getid (n);
	fakt=f(n);
	printid (fakt);
}