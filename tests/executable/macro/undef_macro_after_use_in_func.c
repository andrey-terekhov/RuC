#define name 1

int func()
{
	return name;
}

#undef name

int main()
{
	assert(func() == 1, "Разыменование макроса между использованием в функции и вызовом функции");
	return 0;
}
