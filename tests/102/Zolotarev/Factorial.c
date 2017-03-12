int f(int n)
{
	if (n)
		return n*f(n-1);
	return 1;	
}


void main()
{
	int n;
	getid(n);
	print(f(n));
}	
