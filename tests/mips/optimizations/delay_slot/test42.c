// неправильный вывод
void main()
{
	register int i, j, k = 0;

	for (i = 0; i < 2; ++i)
	{
		for (j = 2; j >= k; --j)
		{
			printid(i, j, k);
		}
	}	
}


