void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncat(&str1, str2, 10);

	printf("%s\n", str1);
	printf("%s\n", str2);
}
