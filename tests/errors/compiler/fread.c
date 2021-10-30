void main()
{
	FILE* f;

	f = fopen("../../../ruc/tests/executable/files/selftest.c", "r+");

	int text[10];
	int a = fread(text, 4, 10, f);
}
