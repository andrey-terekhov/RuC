void main()
{
	char str1[] = "abcd";
	char str2[] = "abc";
	strcat(&str1, str2);

	printf("%s\n", str1);
	printf("%s\n", str2);
}
