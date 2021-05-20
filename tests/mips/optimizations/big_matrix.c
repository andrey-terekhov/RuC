
void main()
{
	int a[200][200], b[200][200], c[200][200];
	register int i, j, k, v;
	for (i = 0; i < 200; ++i)
	{
		for (j = 0; j < 200; ++j)
		{
			a[i][j] = i * j;
		}
	}

	for (i = 0; i < 200; ++i)
	{
		for (j = 0; j < 200; ++j)
		{
			b[i][j] = i + j;
		}
	}

	for (v = 0; v < 1000; ++v)
	{
		for(i = 0; i < 200; ++i)
		{
		    for(j = 0; j < 200; ++j)
		    {
			register int cij = 0;
// если поменять на постинкремент, то будет хуже, надо ли с этим что-то делать?
			for(k = 0; k < 200; ++k)
			    cij += a[i][k] * b[k][j];
			c[i][j] = cij;

		    }
		}
	}
	printf("%i\n", c[0][0]);	
}


