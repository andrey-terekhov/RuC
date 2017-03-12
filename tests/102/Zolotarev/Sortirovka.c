void main()
{
	int i,j,c;
	int a[10];
	getid(a);
	for (i=0; i<9; i++)
		for (j=0; j<9-i; j++)
			if (a[j]>a[j+1])
			{
				c=a[j];
				a[j]=a[j+1];
				a[j+1]=c;
			}
	printid(a);
}