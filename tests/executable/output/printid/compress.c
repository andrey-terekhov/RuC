void main()
{
	int i = 1, и = -1;
	int ident = 32767, идентификатор = -32767;

	char en_symbol = 'Z', ру_символ = 'Я';

	float pi = 3.14;

    struct res
	{
		int x;
		int y;
	} hd = { 1280, 720 };

	int arr[4] = { 0, 1, 2, 3 };


	printid(i);
	printid(i);
	printid(и);
	printid(и);
	printf("\n");

	printid(ident);
	printid(идентификатор);

	printid(en_symbol);
	printid(ру_символ);

	printid(pi);

	printid(hd);

	printid(arr);
}
