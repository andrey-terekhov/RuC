float a;
int b;
void main()
{
    a = sin(3.14/2);
    printid(a);
    a = cos(0);
    printid(a);
    a = sqrt(1.e2);
    print("a 10.000000");
    printid(a);
    a = abs(-2.72);
    print("a 2.720000");
    printid(a);
    a = abs(-3);
    print("a 3.000000");
    printid(a);
    /* sin(3.14/2)=1, cos(0)=1, sqrt(1.e2)=10, abs(-2.72)=2.72, abs(-3)=3 */
    }