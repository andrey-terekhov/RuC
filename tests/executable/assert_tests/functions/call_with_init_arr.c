int func(int *arr)
{
	printf("arr[0] = %i\n", arr[0]);
	printf("arr[1] = %i\n", arr[1]);
	return arr[0] + arr[1];
}

void main()
{
	int a = func({ 1, 2, 3 });
	printf("arr[0] + arr[1] = %i\n", a);
	assert(a == 3, "a != 3");
}
