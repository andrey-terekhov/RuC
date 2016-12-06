float a=2,   b=3;

void main()
{
    int c=4;
    printid(c);         // 4
    b = 5 + a - c + a;
    printid(b);         // 5
    b = a+= c;
    printid(a);         // 6
    printid(b);         // 6
    a = a*b + c/a;
    printid(a);         // 36.666668
}