void main()
{
    int i;
    int  a[3] = {1, 2, 3},
         c[2][3] =  {{1,2,3}, {4, 5, 6}};
    printid(a);
    for (i=0; i<2; i++)
        print(c[i]);
    printid(c);
}
