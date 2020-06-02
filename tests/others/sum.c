int a[5] = {2,3,6,8,9}, b[5] = {3,4,6,9,11}, c[5];
int i;
void main()
{
    for (i=0; i<5; i++)
    {
       c[i] = a[i] + b[i];
    }
printid(c);
    // 5,7,12,17,20
}
