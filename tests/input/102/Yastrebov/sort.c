void main()
{
	float a[10];
	int i;
	int j;
	float m;
	getid (a);
	for(i=0;i<9;i++)
	{
		for(j=0;j<9-i;j++)
		{
			if (a[j]>a[j+1]) 
			{
				m=a[j];
				a[j]=a[j+1];
				a[j+1]=m;
			}
		}
	}
	printid (a);
}