// Проверка на передачу в функию NNP вместо обычного указателя

int func(int *ptr)
{
	int num = *ptr;
	return num;
}

int main()
{
	int num = 9;
	int &pointer = &num;
	func(pointer);
	return 0;
}