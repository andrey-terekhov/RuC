int a[3] = {1,2,3};
int b[4] = {1,2,3,4};
void f()
{
    b[3] = a[2];
}
void main()
{
    printid(a);
    printid(b);
    f();
    printid(b);
}