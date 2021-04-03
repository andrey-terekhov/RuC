int int_func(int num)
{
	return num;
}

float float_func(float num)
{
	return num;
}

void main()
{
	int a = 10;
	float b = 12;

	float arr[] = { 0, 0, 0, 0, 0 };
	arr[0] += 1;
	arr[1] = int_func(2);
	arr[2] += float_func(3.0);
	arr[3] = a - 7;
	arr[4] = a * b - 116;

	assert(arr[0] == 1, "arr[0] != 1");
	assert(arr[1] == 2, "arr[1] != 2");
	assert(arr[2] == 3, "arr[2] != 3");
	assert(arr[3] == 3, "arr[3] != 3");
	assert(arr[4] == 4, "arr[2] != 4");
}
