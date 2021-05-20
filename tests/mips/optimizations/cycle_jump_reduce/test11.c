
void main()
{
	register int i, a, b;

	for (a = 0; a < 10000; ++a)
	{
		for (b = 0; b < 10000; ++b)
		{
			for (i = 0; i < 10; ++i)
				printid(i);
		}
	}	
}


