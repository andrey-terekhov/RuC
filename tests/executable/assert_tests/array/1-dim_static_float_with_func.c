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
	float arr[5] = { int_func(1), int_func(2), int_func(3), float_func(3.0), float_func(4) };

	assert(arr[0] == 1, "arr[0] != 1.0");
	assert(arr[1] == 2, "arr[1] != 2");
	assert(arr[2] == 3, "arr[2] != 3");
	assert(arr[3] == 3, "arr[3] != 3");
	assert(arr[4] == 4, "arr[2] != 4");

	int a = 10;
	float b = 12;
	float arr_2[2] = { int_func(a), float_func(a + b) };

	assert(arr_2[0] == 10, "arr_2[0] != a = 10");
	assert(arr_2[1] == 22, "arr_2[1] != a + b = 22");
}
