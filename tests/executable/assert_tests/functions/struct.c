struct str
{
	int num;
};

int func( struct str st)
{
	return 1 + st.num; 
}

void main()
{
	struct str st =
	{
		1
	};

	assert(func(st) == 2, "1 + st.num = 1 + 1 = 2");
}
