void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncat(&str1, str2, 10);

	int res = strcmp(str1, "abcabcde");
	assert(res == 0, "Результат strncat не соответствует ожидаемому");
}
