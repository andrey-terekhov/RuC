void main()
{
	char str1[] = "abc";
	strncat(&str1, "abcde", 10);

	int res = strcmp(str1, "abcabcde");
	assert(res == 0, "Результат strncat не соответствует ожидаемому");
}
