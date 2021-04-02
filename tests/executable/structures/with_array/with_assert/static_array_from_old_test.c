void main()
{
	char c[2][] = { "abc", "defg" };

	print(c[0][2]);
	assert(c[0][2] == 'c', "c[0] = \"abc\"\nc[0][2] should be 'c'\n");

	print(c[0][1]);
	assert(c[0][1] == 'b', "c[0] = \"abc\"\nc[0][1] should be 'b'\n");
}
