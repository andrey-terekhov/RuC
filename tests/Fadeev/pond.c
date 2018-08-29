#define RED_COLOR 0
#define GREEN_COLOR 1
#define BLUE_COLOR 2
#define X_COMPASS 3
#define Y_COMPASS 4
#define Z_COMPASS 5
#define FI_COMPASS 6

#define LINE 7
#define FLAME 8
#define INFARED 9
#define SOUND 10
#define TOUCH 11
#define ULTRASONIC 12
#define TEMPERATURE 13
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
#define D17 17
#define D18 18
#define D19 19
#define D21 21
#define D22 22
#define D23 23

float pi = 3.141592653589793;

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
