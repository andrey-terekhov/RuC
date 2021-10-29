int main()
{
	int a = 0;
	a = 1 */* long
	comment*/5;

	assert(a == 5, "\"*/* comment */\" should convert to \"*\"");
	return 0;
}
