#define D21 21
#define D22 22
#define RGB 1
#define RELAY 0
#define UVB_ULTRAVIOLET 31
#define UVA_ULTRAVIOLET 30

float pi = 3.141592653589793;

int uva;
int uvb;

/* Threads declarations */
void * thread_1(void *);

/* Threads implementations */
void * thread_1(void * thread_var)
{
	setsignal(RGB, { D22, D21 }, { 0, 0, 0, 0, 0, 255, 255 });
	
	while (1) {
		uva = getdigsensor(UVA_ULTRAVIOLET, { D22, D21 });
		uvb = getdigsensor(UVB_ULTRAVIOLET, { D22, D21 });
		
		draw_number({ D22, D21 }, 0, 0, uva);
		draw_number({ D22, D21 }, 0, 16, uvb);
		t_sleep(500);
		
		clear({ D22, D21 });
	}
	return 0;

}

int main()
{
	
	t_create(thread_1);
	while (1) {
		setsignal(RELAY, { D22, D21 }, { 1, 0, 0, 0 });
		
		t_sleep(1000);
		
		setsignal(RELAY, { D22, D21 }, { 0, 0, 0, 1 });
		
		t_sleep(1000);
		
		setsignal(RELAY, { D22, D21 }, { 0, 1, 0, 0 });
		
		t_sleep(1000);
		
		setsignal(RELAY, { D22, D21 }, { 0, 0, 1, 0 });
		
		t_sleep(1000);
		
	}
	return 0;

}
