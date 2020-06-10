int f(int n)
{	
	if(n <= 0)
        return 1;
    return n * f(n-1);
}


void main()
{	
	int n;
	int fakt;
	getid (n);
	fakt=f(n);
	printid (fakt);
}
