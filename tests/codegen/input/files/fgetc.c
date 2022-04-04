void main()
{
	FILE *f = fopen("../../../ruc/tests/executable/files/selftest.c", "r+");

	char ch = fgetc(f);
	printf("ch = %c", ch);
}
