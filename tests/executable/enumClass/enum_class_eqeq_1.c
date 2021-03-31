// Проверка условного оператора с типом перечисления класса

enum class Color : char {
	red = 'r',
	green = 'g',
	blue = 'b'
};

int main() {
	enum Color clr = Color::red;
	if (clr == Color::green) {
		printf("Что то не так\n");
	} else {
		printf("Действительно blue != green\n");
	}
	return 0;
}