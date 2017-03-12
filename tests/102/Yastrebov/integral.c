void main()
{
	float step,eps,y,x1,s1,x2,s2,x;
	//Вводим концы отрезка, на котором считается интеграл	
	getid (x1);
	printid (x1);
	getid(x2);
	printid (x2);
	eps=1e-4;
	step=0.5;
	do
	{	
		x=x1;
		s1=0;	
		do
		{
			x+=step;
			s1+=sin(x)*step;
		}
		while(x < x2);
		printid (s1);
		
		x=x1;
		step/=5;
		
		s2=0;
		do
		{
			x+=step;
			s2+=sin(x)*step;
		}
		while(x < x2);
        
		step/=5;
		printid (s2);
        printid(step);
	}
	while(abs(s2-s1)>eps);
	print ("Ответ");	
	printid(s2);
}

