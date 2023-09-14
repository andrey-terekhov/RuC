void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncat(&str1, str2, 2);

	int res = strcmp(str1, "abcab");
	assert(res == 0, "Результат strncat не соответствует ожидаемому");
}
