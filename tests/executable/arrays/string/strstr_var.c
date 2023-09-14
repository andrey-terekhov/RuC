void main()
{
	char str1[] = "abcdef";
	char str2[] = "ab";
	char str3[] = "cd";
	char str4[] = "hg";
	char str5[] = "abcdefg";

	int res;
	res = strstr(str1, str2);
	assert(res == 1, "sstrstr не нашел ab в abcdef");
	
	res = strstr(str1, str3);
	assert(res == 3, "sstrstr не нашел cd в abcdef");
	
	res = strstr(str1, str4);
	assert(res == -1, "sstrstr нашел hg в abcdef");
	
	res = strstr(str1, str5);
	assert(res == -1, "sstrstr нашел abcdefg в abcdef");
}
