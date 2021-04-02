void main()
{
	float arr[5] = { 1.0, 2, 3 / 5, 1 + 1, 1.0 * 1 };

	assert(arr[0] == 1, "arr[0] != 1.0");
	assert(arr[1] == 2, "arr[1] != 2");
	assert(arr[2] == (3 / 5), "arr[2] != 3 / 5");
	assert(arr[3] == 2, "arr[3] != 2");
	assert(arr[4] == 1, "arr[4] != 1");

	int a = 10;
	float b = 12;
	float arr_2[6] = { a, a + b, a * b, exp(a), 110 / a, 1 + b };

	assert(arr_2[0] == 10, "arr_2[0] != a = 10");
	assert(arr_2[1] == 22, "arr_2[1] != a + b = 22");
	assert(arr_2[2] == 120, "arr_2[2] != a * b = 120");
	assert(arr_2[3] == exp(10), "arr_2[3] != exp(a) = exp(10)");
	assert(arr_2[4] == 11, "arr_2[4] != 110 / a = 11");
	assert(arr_2[5] == 13, "arr_2[5] != 1 + b = 13");
}
