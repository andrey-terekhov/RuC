void MAIN()
{
	int a;

	int *p1 = NULL;
	int *p2 = &a;

	if (p1)
	{
		assert(0, "p1 != NULL");
	}

	if (!p2)
	{
		assert(0, "p2 == NULL");
	}

	if (!p1 && p2 && (p2 != NULL) && (p1 == NULL))
	{
		return;
	}
	else
	{
		assert(0, "wrong pointer comparison");
	}
}
