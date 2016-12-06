int j=-2, i=j, a=2*i-j;
void f();
int main()
{
    for(i=0; i<=10; i+=2)
    {
        j+=i;
    };
    a<<=2;
    while(a>j)
    {
        f();
        a++;
    }
}
void f()
{
    for ((i=0, j=9); i<10 && j>-i; (i++, j--))
    {
        a+=i+j;
    }
}
