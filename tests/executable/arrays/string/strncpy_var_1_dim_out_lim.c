void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncpy(&str1, str2, 10);

	int res = strcmp(str1, "abcde");
	assert(res == 0, "Результат strncpy не соответствует ожидаемому");
}
