float pi = 3.141592653589793;

int main()
{
	
	while (1) 
	{
		clear({22, 21});
		draw_string({22, 21}, 0, 0, "Hello world!");
		t_sleep(1000);
		
		clear({22, 21});
		draw_number({22, 21}, 0, 0, 0.5);
		t_sleep(1000);
		
		clear({22, 21});
		pixel({22, 21}, 5, 5);
		line({22, 21}, 0, 0, 10, 10);
		t_sleep(1000);
		
		clear({22, 21});
		rectangle({22, 21}, 0, 0, 5, 5, 0);
		ellipse({22, 21}, 0, 0, 50, 5, 0);
		t_sleep(1000);
		
		clear({22, 21});
		icon({22, 21}, 5, 5, 0);
		t_sleep(1000);
		
	}

	return 0;
}
