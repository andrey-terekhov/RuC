#define MA 0
#define MB 1
#define D22 22
#define D21 21
#define D16 16
#define D17 17

#define LASER 13
#define X_GYROSCOPE 17

float pi = 3.141592653589793;

float angle;
int laser;
float x;
float y;
float z;

int main()
{
	
	while (1) {
		laser = getdigsensor(LASER, { D22, D21 });
		
		if (laser > 100) {
			setmotor(MA, 50);
			setmotor(MB, 50);
			
		} else {
			angle = 90;
			do {
				setmotor(MA, 50);
				
				setmotor(MB, -(50));
				
				t_sleep(100);
				
				x = getdigsensor(X_GYROSCOPE, { D17, D16 }) / 100;
				
				angle = angle - abs(x) / 10;
			} while (angle > 0);
			setmotor(MA, 50);
			setmotor(MB, 50);
			
		}
		t_sleep(1000);
		
	}
	return 0;

}
