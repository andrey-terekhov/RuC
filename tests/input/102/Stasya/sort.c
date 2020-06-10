int n, i, j, t;
void main()
{
	getid(n);
	printid(n);
	{
	int a[n];
	getid(a);
	printid(a);
	for (j=1; j<n; j++)
		for(i=0; i<n-j; i++)
		if(a[i]>a[i+1])
		{
			t=a[i];
			a[i]=a[i+1];
			a[i+1]=t;
		}
	printid(a);
	}
}