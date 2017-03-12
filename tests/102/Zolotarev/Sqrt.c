float eps=1e-6;

void main()
{
	float x, a;
	getid(a);
	x=a;
	while (abs(x*x-a)>eps)
	{
		x=0.5*(x+a/x);
	}
	printid(x);
}