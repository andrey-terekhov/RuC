void main()
{
    int a[5],max,i;
    getid (a);
    max=a[0];
    for (i=1; i<5; i++)
        if (max<a[i])
            max=a[i];
    printid(max);
}