int a=1,b=2;
int f(), q(int, int);
int f()
{
    return a;
}
int q(int x, int y)
{
    return x+y;
}
void main()
{
    print(f()+3);    // 4
    print(q(a+b,2)); // 5
}