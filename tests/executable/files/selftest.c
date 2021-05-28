void main()
{
	FILE f = fopen("/Users/ant/RuC/tests/executable/files/selftest.c");

	char ch = fgetc(f);
	while (ch != -1)
	{
		printf("%c", ch);
		ch = fgetc(f);
	}
	
	fclose(f);
}
