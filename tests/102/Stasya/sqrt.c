void main ()
{
float a, x;
getid(a);
printid(a);
x=a;
while (abs(x*x-a)>1e-6)
x=(x+a/x)/2;
printid(x);
}