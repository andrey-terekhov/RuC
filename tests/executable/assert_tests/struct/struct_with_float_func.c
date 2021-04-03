float float_func(int num)
{
	return num;
}

void main()
{
	struct str
	{
		int i;
		float f;
		double d;
	};

	struct str st =
	{
		8,
		float_func(2),
		float_func(3)
	};
	assert(st.i == 8, "st.i != 8");
	assert(st.f == 2, "float init by float function");
	assert(st.d == 3, "double init by float function");
	
	st.i = 10;
	st.f = float_func(3);
	st.d = float_func(4);
	assert(st.i == 10, "int redef by int");
	assert(st.f == 3, "float redef by float function");
	assert(st.d == 4, "double redef by float function");
}
