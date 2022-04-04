typedef int elem;

int main()
{
	elem mas[2];
	mas[0] = 0;
	mas[1] = 1;

	assert(mas[0] == 0, "mas[0] != 0");
	assert(mas[1] == 1, "mas[1] != 1");
	return 0;
}
