
void main()
{
	register int i, j;
	int m = 5, n = 5;
	int a[m][n];

	for (i = 0; i < 5; ++i)
	{
		for (j = 0; j < 5; ++j)
		{
			a[j][i] = i + j;
		}
	}
	printid(a);
}


