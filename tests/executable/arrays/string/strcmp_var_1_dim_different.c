void main()
{
	char str1[] = "abcd";
	char str2[] = "abc";

	int res = strcmp(str1, str2);
	assert(res == -1, "strcmp для различных строк вернул не -1");
}
