// Проверка условного оператора с типом перечисления

enum Color : char
{
	red = 'r',
	green = 'g',
	blue = 'b'
};

int main()
{
	enum Color clr = red;
	if (clr == green)
	{
		printf("Что то не так\n");
	}
	else
	{
		printf("Действительно blue != green\n");
	}
	return 0;
}