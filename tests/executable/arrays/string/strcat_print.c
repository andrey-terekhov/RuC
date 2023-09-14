void main()
{
	char str1[] = "abcd";
	char str2[] = "abc";
	strcat(&str1, str2);

	print(str1);
	print(str2);
}
