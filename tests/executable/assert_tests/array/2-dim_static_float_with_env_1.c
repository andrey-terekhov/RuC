void main()
{
	float arr[2][5] = 
	{
		{ 1.0, 2, 3 / 5, 1 + 1, 1.0 * 1 },
		{ 1.0, 2, 3 / 5, 1 + 1, 1.0 * 1 }
	};

	assert(arr[0][0] == 1, "arr[0] != 1.0");
	assert(arr[1][1] == 2, "arr[1] != 2");

	int a = 10;
	float b = 12;
	float arr_2[6][2] =
	{
		{ a, a + b, a * b, exp(a), 110 / a, 1 + b },
		{ a, a + b, a * b, exp(a), 110 / a, 1 + b }
	};

	assert(arr_2[0] == 10, "arr_2[0] != a = 10");
	assert(arr_2[1][1] == 22, "arr_2[1] != a + b = 22");
}
