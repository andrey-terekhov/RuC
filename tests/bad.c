float f(float x)
{
    return sin(x);
}
float a, b, t, i;
//float eps=1e-6;
int n=10;
float l=(b-a)/n;
float sum=0;

void main ()
{
    getid(a);
    printid(a);
    getid(b);
    printid(b);
    if (a>b)
    {
        t=a;
        a=b;
        b=t;
    }
    for (i=a; i<b; i+=l)
    {
        printid(i);
        printid(b);
        printid(l);
        sum+=f(i)*l;
        printid(sum);
    }
}
