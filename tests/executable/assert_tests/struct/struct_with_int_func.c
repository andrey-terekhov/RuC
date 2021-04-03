int int_func(int num)
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
		int_func(1),
		int_func(2),
		int_func(3)
	};
	assert(st.i == 1, "int init by int function");
	assert(st.f == 2, "float init by int function");
	assert(st.d == 3, "double init by int function");

	st.i = int_func(2);
	st.f = int_func(3);
	st.d = int_func(4);
	assert(st.i == 2, "int redef by int function");
	assert(st.f == 3, "float redef by int function");
	assert(st.d == 4, "double redef by int function");
}
