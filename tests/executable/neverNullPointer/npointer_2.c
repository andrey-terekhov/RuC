// Проверка работы * к Never NULL pointer

enum class Color_test_1 : int {
	red = 5,
	green,
	blue
};

enum Color_test_2 : int {
	red = 10,
	green,
	blue
};

int main() {
	int first_test_1 = Color_test_1::red;
	int first_test_2 = blue;

	int &pointer_test_1 = &first_test_1;
	int &pointer_test_2 = &first_test_2;

	int second_test_1 = *pointer_test_1;
	int second_test_2 = *pointer_test_2;
	return 0;
}