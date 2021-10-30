float f(float x)
{
	return sin(x);
}
float a, b, t=1, l;
int i;
int n=10;
float sum=0;
void main ()
{
	getid(a);
	printid(a);
	getid(b);
	printid(b);
	{
        while (abs(t-sum) > 1e-6)
        {
            l=abs(b - a) / n;
            t=sum;
            sum=0;
            for (i=0; i * l < abs(b - a); i++)
                sum+=((f(a+i*l) + f(a+(i+1)*l))) * l/2;
//            printid(n);
//            printid(sum);
            n *= 2;
        }	
        printid(sum);
    }
}
