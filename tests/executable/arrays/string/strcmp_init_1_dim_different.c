void main()
{
	int res = strcmp("abc", "abcd");
	assert(res == -1, "strcmp для различных строк вернул не -1");
}
