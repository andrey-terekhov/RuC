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

#define M0 0

#define M1 1

#define A0 -1

#define A1 -2

#define A2 -3

#define A3 -4

#define A4 -5

#define A5 -6

#define A6 -7

#define A7 -8

#define D0 0

#define D1 1

#define D2 2

#define D3 3

#define D4 4

#define D5 5

#define D6 6

#define D7 7

float pi = 3.141592653589793;

int main()
{
	
	while (!(getansensor(INFARED, A0) > 0)) {
		t_sleep(10);
	}
	
	return 0;

}
