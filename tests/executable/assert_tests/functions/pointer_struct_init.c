struct str
{
	int num;
};

int func( struct str *st)
{
	st->num += 1;
	return 1 + st->num; 
}

void main()
{
	assert(func({ 1 }) == 3, "1 + st.num + 1 = 1 + 2 = 3");
}
