void main()
{
	char str1[] = "abc";
	strncpy(&str1, "abcde", 10);

	int res = strcmp(str1, "abcde");
	assert(res == 0, "Результат strncpy не соответствует ожидаемому");
}
