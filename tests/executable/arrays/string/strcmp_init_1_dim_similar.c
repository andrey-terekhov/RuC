void main()
{
	int res = strcmp("abcd", "abcd");
	assert(res == 0, "strcmp для одинаковых строк вернул не 0");
}
