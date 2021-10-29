int main()
{
	int a = 0;
	a = 1 *// short comment
	5;

	assert(a == 5, "\"*//\" should convert to \"*\"");
	return 0;
}
