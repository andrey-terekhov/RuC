int main()
{
	int ret = 0;

//	ret = 1;
	assert(ret == 0, "short comment at the begining");//ret = 1;
	assert(ret == 0, "short comment in the middle");

/*	assert(0, "long comment at the beginning");*/ret = 1/*ret = 3*/;
	assert(ret == 1, "long comment in the middle");

	ret/* = 1; assert(0, "long comment");
	char str[] = "Hello world";
	*/= 2;
	assert(ret == 2, "3 str comment: begin in middle, end in middle");

	/*long comment //short comment*/ret = 4;
	assert(ret == 4, "short comment ib long comment");

	/*assert(0, "long comment in long comment 1");
	/*assert(0, "long comment in long comment 2");
	*/

	// short comment /*
	ret = 5;
	/* long comment*/
	assert(ret == 5, "long comment begin in short comment");

	/*long comment // short comment */ ret = 6;
	assert(ret == 6, "long comment end in short comment");

	return 0;
}
