void main()
{
	char c[2][] = { "abc", "defg" };
	struct students { char names[]; int marks[]; } st[] =
	{
		{ "stud1", { 1, 2, 3 } },
		{ "stud2", { 3, 4 } }
	};

	print(c[0][2]);
	assert(c[0][2] == 'c', "c[0] = \"abc\"\nc[0][2] should be 'c'\n");

	print(c[0][1]);
	assert(c[0][1] == 'b', "c[0] = \"abc\"\nc[0][1] should be 'b'\n");

	print(st[1].names[3]);
	assert(st[1].names[3] == 's', "st[1].names = \"stud2\"\nst[1].names[2] shold be 'd'\n");
}
