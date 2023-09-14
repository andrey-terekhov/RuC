void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strncpy(&str1, str2, 2);

	print(str1);
	print(str2);
}
