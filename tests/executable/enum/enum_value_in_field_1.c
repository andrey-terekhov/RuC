// Проверка значения полей в типе перечисления

enum Color : int { r = 5, g, b = 2};

int main() {
	int i = g;
	print(i);
	return 0;
}