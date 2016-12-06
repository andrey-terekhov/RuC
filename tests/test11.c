int a=1, b=2;
int f()
{
   return a+b;
}
int q(int x, int y)
{
    int u =5;
    return x + y + f() + u;
}
void main()
{
    int ans;
    ans = q(3,4);
    printid(ans);  // 15
    return;
}