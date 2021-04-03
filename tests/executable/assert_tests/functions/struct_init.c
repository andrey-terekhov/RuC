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
	assert(func({ 1 }) == 2, "1 + st.num = 1 + 1 = 2");
}
