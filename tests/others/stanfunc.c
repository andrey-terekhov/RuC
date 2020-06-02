float a;
int b;
void main()
{
    a = sin(3.14/2);
    printid(a);
    a = cos(0);
    printid(a);
    a = sqrt(1.e2);
    printid(a);
    a = abs(-2.72);
    printid(a);
    a = abs(-3);
    printid(a);
    /* sin(3.14/2)=1, cos(0)=1, sqrt(1.e2)=10, abs(-2.72)=2.72, abs(-3)=3 */
    print(rand());
    print(rand());
}
   
