// Проврка оператора =, для базового типа поля перечисления

enum class car : char
{
	a = '1',
	b,
	c = '0'
} tmp;

struct test
{
	int a;
};

int main()
{
	enum car i = car::b;
	return 0;
}