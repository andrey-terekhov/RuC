#define WATER 14
#define SOIL 15
#define FLOW 16

#define MA -1
#define MB -2
#define A4 32
#define A5 33
#define A6 34
#define A7 35
#define A18 25
#define A19 26
#define D5 5
#define D12 12
#define D13 13
#define D14 14
#define D15 15
#define D16 16
#define TEMPERATURE 20

int U;
int k;
int s1;
int s2;
int tmax;
int tmin;
int water;

/* Threads declarations */
void * thread_1(void *);

/* Threads implementations */
void * thread_1(void * thread_var)
{
	k = 0;
	s1 = 0;
	s2 = 0;
	while (1) {
		water = getdigsensor(WATER, { D14 });
		
		if (water == 0) {
			setmotor(MA, 0);
			
			setmotor(MB, 50);
			
			t_sleep(1000);
			
		} else {
			s1 = getansensor(FLOW, A5);
			
			s2 = getansensor(FLOW, A4);
			
			U = s2 - s1;
			setmotor(MA, 50 + k * U);
			
			setmotor(MB, 50 - k * U);
			
			t_sleep(1000);
			
		}
	}
	return 0;

}

int main()
{
	
	t_create(thread_1);
	tmin = 30;
	tmax = 60;
	while (1) {
		setvoltage(D15, 0);
		
		setvoltage(D16, 0);
		
		while (!(getansensor(TEMPERATURE, A7) < tmin)) {
			t_sleep(10);
		}
		
		setvoltage(D15, 255);
		
		setvoltage(D16, 255);
		
		while (!(getansensor(TEMPERATURE, A7) > tmax)) {
			t_sleep(10);
		}
		
	}
	return 0;

}
