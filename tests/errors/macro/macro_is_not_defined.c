#define name 10
#undef name

int main()
{
	print(name);	// идентификатор name неопределен
	return 0;
}
