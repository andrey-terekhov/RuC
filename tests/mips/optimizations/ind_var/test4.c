
void main()
{
	register int i, j;
	int a[5][5];

	for (i = 0; i < 5; ++i)
	{
		for (j = 0; j < 5; ++j)
		{
			a[j][i] = i + j;
		}
	}
	printid(a);
}


