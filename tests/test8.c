int a=2, b=3;
//float b1=1.23;
void main()
{
    int c=4;
//    {int b = a+= c; c^=7;printid(c);}
    b = (4 + a + c) % a;
    printid(b);       // 0
}
