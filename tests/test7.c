int a=2,   b=3;

void main()
{
    int c=4;
    b = 5 + a - c + a;
    printid(b);
    b = a+= c;
    printid(a);
    printid(b);
    a = a*b + c/2;
    printid(a);
}
