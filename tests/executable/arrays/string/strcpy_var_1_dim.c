void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strcpy(&str1, str2);

	int res = strcmp(str1, "abcde");
	assert(res == 0, "Результат strcpy не соответствует ожидаемому");
}
