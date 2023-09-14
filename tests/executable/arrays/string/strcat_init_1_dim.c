void main()
{
	char str1[] = "abcd";
	strcat(&str1, "abc");

	int res = strcmp(str1, "abcdabc");
	assert(res == 0, "Результат strcat не соответствует ожидаемому");
}
