void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncpy(&str1, str2, 2);

	printf("%s\n", str1);
	printf("%s\n", str2);
}
