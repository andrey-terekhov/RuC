void main()
{
	char str1[] = "abcdef";

	int res;
	res = strstr(str1, "ab");
	assert(res == 1, "strstr не нашел ab в abcdef");
	
	res = strstr("qwabcdef", str1);
	assert(res == 3, "strstr не нашел abcdef в qwabcdef");
	
	res = strstr("abcdef", "hg");
	assert(res == -1, "strstr нашел hg в abcdef");
	
	res = strstr("abcdef", "bc");
	assert(res == 2, "strstr не нашел bc в abcdef");
}
