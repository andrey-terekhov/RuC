int a,   b;

float b1 =3.14;


void main()
{
    int c = 1<a ? a : 2>b ? b : c;
    b = 1<a ? a : 2>b ? b : b1;
    b = 1<a ? a : 2>b ? b1 : b;
}