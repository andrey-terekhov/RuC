int a,b[3]={7,8,9};
int i=2;
void main()
{
b[i] -= b[1]+=i;
    printid(b);   // 7 10 -1
}
