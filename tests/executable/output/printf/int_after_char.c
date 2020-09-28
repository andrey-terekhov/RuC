int main()
{
	char ch = '1';
	int num = 2;

	printf("local = %i\n", num);
	printf("char is %c & now local is %i\n", ch, num);

	printf(" <- here shouldn't be anything (only 1 copy)!!!\n");
	printf("This is the last string, without line break");

	return 0;
}
