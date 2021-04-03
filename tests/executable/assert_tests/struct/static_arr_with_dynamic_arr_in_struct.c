void main()
{
	struct students { char names[]; int marks[]; } st[2] =
	{
		{ "stud1", { 1, 2, 3 } },
		{ "stud2", { 3, 4 } }
	};

	print(st[1].names[3]);
	assert(st[1].names[3] == 's', "st[1].names = \"stud2\"\nst[1].names[2] shold be 'd'\n");
}
