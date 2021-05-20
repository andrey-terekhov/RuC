
void main()
{
	register int i;
	int a[5], b[5];

	for (i = 0; i < 5; ++i)
	{
		a[i] = i;
		b[i] = i + 1;
	}
	printid(a);
	printid(b);
}


