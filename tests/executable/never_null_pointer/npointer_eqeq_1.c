enum Color : char { red = 'r', green = 'g', blue = 'b' } car, house;

int main()
{
	char num1 = red, num2 = red;
	char &poiner_1 = &num1;
	char &poiner_2 = &num2;
	return *poiner_1 != *poiner_2;
}
