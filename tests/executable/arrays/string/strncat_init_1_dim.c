void main()
{
	char str1[] = "abc";
	strncat(&str1, "abcde", 2);

	int res = strcmp(str1, "abcab");
	assert(res == 0, "Результат strncat не соответствует ожидаемому");
}
