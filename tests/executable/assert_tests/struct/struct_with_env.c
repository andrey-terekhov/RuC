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
		10 / 2,
		11.0 + 0.1,
		exp(1)
	};
	assert(st.i == 5, "int init by enviroment");
	assert(st.f == 11.1, "float init by enviroment");
	assert(st.d == exp(1), "double init by enviroment");

	int a = 11;
	st.i += 1;
	st.f = st.i * 10;
	st.d = a + 1;
	assert(st.i == 6, "int redef by enviroment");
	assert(st.f == 60, "float redef by enviroment");
	assert(st.d == 12, "double redef by enviroment");
}
