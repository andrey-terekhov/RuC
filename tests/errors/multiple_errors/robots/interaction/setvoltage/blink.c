#define D13 13
#define D14 14
float pi = 3.141592653589793;

int main()
{
	
	while (1) {
		setvoltage(D14, 255);
		
		setvoltage(D13, 128);
		
		t_sleep(1000);
		
		setvoltage(D14, 0);
		
		setvoltage(D13, 0);
		
		t_sleep(1000);
		
	}

	return 0;

}
