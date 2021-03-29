void main()
{
	// int err[];
	// int one[1];
	// int dyn[] = { 1, 2 };
	// int stat[2] = { 1, 2 };

	struct students_1 { char names[2]; };
	struct students_1 st_1;
	st_1.names[0] = 'w';
	print(st_1.names[0]);
	assert(st_1.names[0] == 'w', "st_1.names[0] should be 'w'\n");

	struct students_2 { char names[]; };
	struct students_2 st_2;
	st_2.names[0] = 'w';
	print(st_2.names[0]);
	assert(st_2.names[0] == 'w', "st_2.names[0] should be 'w'\n");
}
