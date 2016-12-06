int f(int);
int f(int x)
{
    return x;
}
float f1(float);
float f1(float y)
{
    return y;
}
float q(int, float, int(*)(int), float(*)(float));
float q(int i, float r, int(*ff)(int), float(*f1f)(float))
{
    return ff(i) + f1f(r);
}
void main()
{
    print(q(2, 1.14, f, f1)); // 3.14
}