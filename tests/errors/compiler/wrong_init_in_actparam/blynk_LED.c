#define V0 0
#define V1 1
#define V2 2

#define D21 21
#define D22 22

#define RGB 1

float pi = 3.141592653589793;

int blue;
int green;
int red;

int main()
{
	
	wifi_connect("wifi", "pass");
	
	blynk_authorization("auth");
	
	while (1) {
		red = blynk_receive(V0);
		
		green = blynk_receive(V1);
		
		blue = blynk_receive(V2);
		
		setsignal(RGB, { D22, D21 }, { red, green, blue, 0, 0, 0, 0 });
		
		t_sleep(250);
		
	}
	return 0;

}
