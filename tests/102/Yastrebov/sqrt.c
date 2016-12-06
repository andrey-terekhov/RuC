void main()
{
	float a,x,e;
	e=1e-5;
	getid (a);
	x=a/2;
	do
		x=(1.0/2.0)*(x+a/x);
	while (abs(x*x)-a>e);
	printid (x);
}