int func(int *arr)
{
	return arr[0] + arr[1];
}

void main()
{
	int a = func({ 1, 2, 3 });
	assert(a == 3, "a != 3");
}
