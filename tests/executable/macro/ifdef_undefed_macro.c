#define name
#undef name

#ifdef name 
	assert(1 == 0, "Эта часть кода должна быть отрезана");
#endif

int main()
{
	return 0;
}
