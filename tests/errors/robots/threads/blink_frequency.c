#define D13 13
#define D14 14
float pi = 3.141592653589793;

/* Threads implementations */
void* thread_1(void* v)
{
	while (1) {
		setvoltage(D14, 255);
		
		t_sleep(1000);
		
		setvoltage(D14, 0);
		
		t_sleep(1000);
		
	}
    
    return 0;

}

int main()
{
	
	t_create(thread_1);
	while (1) {
		setvoltage(D13, 255);
		
		t_sleep(250);
		
		setvoltage(D13, 0);
		
		t_sleep(250);
		
	}

	return 0;

}
