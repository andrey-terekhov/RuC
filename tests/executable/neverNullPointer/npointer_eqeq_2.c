enum class Color : char { red = 'r', green = 'g', blue = 'b' } car, house;

int main() {
	char num1 = Color::red, num2 = Color::red;
	char &poiner_1 = &num1;
	char &poiner_2 = &num2;
	return *poiner_1 != *poiner_2;
}
