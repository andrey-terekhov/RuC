#include <stdio.h>
#include <math.h> 

#define BUFF_SIZE 20

char INPUT_BUFF[BUFF_SIZE+1];

int parse_number(float* ptr_output_number)
{
	char* stack_buffer[BUFF_SIZE+1];

	int i = 0;
	int sign = 1;
	int exponent = 0;

	int state = 0;
	/*
		0 - read first symbol
		1 - read integer part of number
		2 - read fraction part of number
	*/

	for (; INPUT_BUFF[i] != '\n'; ++i)
	{
		switch (state)
		{
			case 0:
				if (INPUT_BUFF[i] == '-')
				{
					sign = -1;
					state = 1;
					break;
				}

				if (INPUT_BUFF[i] >= '0' && INPUT_BUFF[i] <= '9')
				{
					*ptr_output_number = INPUT_BUFF[i] - '0';
					state = 1;
					break;
				}
				else
				{
					return i+1;   // Error
				}

			case 1:
				if (INPUT_BUFF[i] == '.')
				{
					state = 2;
					exponent = -1;
					break;
				}

				if (INPUT_BUFF[i] >= '0' && INPUT_BUFF[i] <= '9')
				{
					*ptr_output_number = 
					*ptr_output_number*10 + INPUT_BUFF[i] - '0';
					break;
				}
				else
				{
					return i+1;   // Error
				}

			case 2:
				if (INPUT_BUFF[i] >= '0' && INPUT_BUFF[i] <= '9')
				{
					*ptr_output_number = 
					*ptr_output_number + (INPUT_BUFF[i] - '0')*pow(10,exponent);
					exponent--;
					break;
				}
				else
				{
					return i+1;   // Error
				}
		}
	}

	*ptr_output_number = *ptr_output_number*sign;

	return 0;
}

void show_error_messsage(int error_code)
{
	printf("Parse error: in symbol %c at %d position\n", 
		INPUT_BUFF[error_code], error_code);
}

int main(int argc, char const *argv[])
{
	printf("Maximum digits in number: %d\nTo change this number edit souce code\n", 
		BUFF_SIZE);

	float result_number = 0;

	fgets(INPUT_BUFF, sizeof INPUT_BUFF, stdin);

	INPUT_BUFF[BUFF_SIZE] = '\n';      // provide valid line	

	int result_code = parse_number(&result_number);

	if (result_code)
	{
		show_error_messsage(result_code);
	}
	else
	{
		printf("Result: %f\n", result_number);
	}

	return 0;
}