int func()
{
	int name = 2;
#define name 1
	return name;
}

int main()
{
	assert(func() == 1, "После инициализации переменной был создан макрос");
	return 0;
}
