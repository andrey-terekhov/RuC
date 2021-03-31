// Проверка анонимных перечислений класса с новым базовым типом для полей

enum class : char
{
	a,
	b,
	c
} tmp;

int main()
{
	char num = tmp::a;
	return 0;
}