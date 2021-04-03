void main()
{
	struct students_2 { char names[2]; };
	
	struct students_2 st_2;
	st_2.names[0] = 'w';
	
	print(st_2.names[0]);
	assert(st_2.names[0] == 'w', "st_2.names[0] should be 'w'\n");
}
