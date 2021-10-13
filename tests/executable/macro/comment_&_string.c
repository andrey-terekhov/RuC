int main()
{
	int ret;

	char str1[] = "abc";
	ret = strcmp(str1, "abc");
	assert(ret == 0, "string const");

	char str2[] = "ab//";
	ret = strcmp(str2, "ab//");
	assert(ret == 0, "short comment begin in string");

	char str3[] = "ab/*q";
	ret = strcmp(str3, "ab/*q");
	assert(ret == 0, "long comment begin in string");

	char str4[] /*= "ab*/= "q";
	ret = strcmp(str4, "q");
	assert(ret == 0, "long comment end in string");

	return 0;
}
