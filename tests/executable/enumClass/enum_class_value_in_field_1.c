// Проверка значения полей в типе перечисления класс

enum class Color : int { r = 5, g, b = 2};

int main() {
	int i = Color::b;
	printf("%d\n", i);
	return 0;
}