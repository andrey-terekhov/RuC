// Проврка оператора =, для базового типа поля перечисления

enum car : char
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
	enum car i = c;
	return 0;
}