struct str
{
	int num;
	int arr[2];
};

int func( struct str *st)
{
	st->num += 1;
	return 1 + st->num; 
}

void main()
{
	struct str st =
	{
		1,
		{ 1, 2 }
	};

	assert(func(&st) == 3, "1 + st.num + 1 = 1 + 2 = 3");
	assert(st.num == 2, "after func st.num shuld be 2");
}
