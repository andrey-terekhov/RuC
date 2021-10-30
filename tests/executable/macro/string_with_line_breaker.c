int main()
{
	int ret;

	char str[] = "long\
 string";
	ret = strcmp(str, "long string");

	assert(ret == 0, "line breaker in string is not working");
	return 0;
}
