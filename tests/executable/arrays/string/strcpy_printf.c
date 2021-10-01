void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strcpy(&str1, str2);

	printf("%s\n", str1);
	printf("%s\n", str2);
}
