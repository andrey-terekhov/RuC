
void main()
{
	register int i, j, k = 2;

	for (i = 0; i < 2; ++i)
	{
		for (j = 8; k * 3 < j; --j)
		{
			printid(i, j, k);
		}
	}	
}


