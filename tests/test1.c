int a[6];
int j, i, h;

void main ()
{
    getid(a);
    for (i = 0; i < 6; i++)
        for (j = 0; j < 6-i; j++)
            if (a[j] > a[j+1])
            {
                h = a[j];
                a[j] = a[j+1];
                a[j+1] = h;
            }
    for (i = 0; i < 6; i++)
        printid(a[i]);
}