void main()
{
	int a = 0;
	for (int i = 0; i < 10; i++)
		a++;
	assert(a == 10, "Must be 10");

	a = 0;
	for (int i = 0; i < 10; i++)
	{
		a++;
	}
	assert(a == 10, "Must be 10");
}
