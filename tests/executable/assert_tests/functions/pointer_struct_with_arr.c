struct str
{
	int num;
	int arr[2];
};

int func( struct str *st)
{
	return st->arr[1] + st->num; 
}

void main()
{
	struct str st =
	{
		1,
		{ 1, 2 }
	};

	assert(func(&st) == 3, "st.arr[1] + st.num = 2 + 1 = 3");
}
