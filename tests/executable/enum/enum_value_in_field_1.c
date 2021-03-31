// Проверка значения полей в типе перечисления

enum Color : int { r = 5, g, b = 2};

int main() {
	int i = g;
	printf("%d\n", i);
	return 0;
}