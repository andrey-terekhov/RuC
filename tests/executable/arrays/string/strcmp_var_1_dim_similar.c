void main()
{
	char str1[] = "abcd";
	char str2[] = "abcd";

	int res = strcmp(str1, str2);
	assert(res == 0, "strcmp для одинаковых строк вернул не 0");
}
