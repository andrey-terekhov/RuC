void main()
{
	int i, k;
	float j, c;
	char b[50];
	char a[50][31];
	for (i=0; i<50; i++)
		for (k=0; k<31; k++)
			a[i][k]=' ';		
	for (i=0; i<50; i++)
	{
		j=i*0.3;
		c=15*sin(j); 	
		k=round(c);
		a[i][k+15]='X';
	}
	for (i=0; i<50; i++)
	{
		for (k=0; k<31; k++)
		{
			b[k]=a[i][k];
		}
		printid(b);
	}
		
}
		
	