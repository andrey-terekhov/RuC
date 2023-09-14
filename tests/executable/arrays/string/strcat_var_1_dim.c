void main()
{
	char str1[] = "abcd";
	char str2[] = "abc";
	strcat(&str1, str2);

	int res = strcmp(str1, "abcdabc");
	assert(res == 0, "Результат strcat не соответствует ожидаемому");
}
