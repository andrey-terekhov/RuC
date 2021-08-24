void MAIN()
{
	int a;

	int *p1 = nullptr;
	int *p2 = &a;

	if (p1)
	{
		assert(0, "p1 != nullptr");
	}

	if (!p2)
	{
		assert(0, "p2 == nullptr");
	}

	if (!p1 && p2 && (p2 != nullptr) && (p1 == nullptr))
	{
		return;
	}
	else
	{
		assert(0, "wrong pointer comparison");
	}
}
