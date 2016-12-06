int find(int val)
{
    int a[11];
    getid(a);
    int i,s=0;
    for (i=1;i<=9;i+=2)
    {
        if (a[i]==val)
        {
            printid(i);
            break;
            s=1;
        }
    }
}

void main()
{
    int b=4;
    if (s==1)
    {
         find(b);
    }
    else
    {
        printid(0);
    }
}

