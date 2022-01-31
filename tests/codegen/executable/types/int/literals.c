void main()
{
	assert(0xAf514 == 718100, "wrong hexadecimal reading");
	assert(0XFE9584 == 16684420, "wrong hexadecimal reading");

	assert(0b10110 == 22, "wrong binary reading");
	assert(0B10011 == 19, "wrong binary reading");

	assert(0o10110 == 4168, "wrong octal reading");
	assert(0O10011 == 4105, "wrong octal reading");

	assert(0d5678124 == 5678124, "wrong decimal reading");
	assert(0D5671244 == 5671244, "wrong decimal reading");
}
