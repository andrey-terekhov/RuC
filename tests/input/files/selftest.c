void main()
{
	FILE *f = fopen("../../../ruc/tests/executable/files/selftest.c", "r+");

	char ch = fgetc(f);
	while (ch != -1)
	{
		printf("%c", ch);
		ch = fgetc(f);
	}
	
	fclose(f);
}
