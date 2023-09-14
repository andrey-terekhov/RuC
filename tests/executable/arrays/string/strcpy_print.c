void main()
{
	char str1[] = "abc";
	char str2[] = "abcde";
	strcpy(&str1, str2);

	print(str1);
	print(str2);
}
