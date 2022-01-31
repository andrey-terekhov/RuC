int GLOBAL = 42;
char numbers[5] = {'1', '2', '3', '4', '5'};


int main()
{
	int local = 2;

	printf("Local variable = %i\n", local);
	printf("Global char array with local index var is %c & global var is %i\n", numbers[local], GLOBAL);

	printf(" <- here shouldn't be anything (only 1 copy)!!!\n");
	printf("This is the last string, without line break");

	return 0;
}
