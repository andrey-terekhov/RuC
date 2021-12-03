float a[5] = {0, 1, 1488, -2.28, 100}; //максимальное число 1488

void main()
{
    int i, j = 0;
    float x=a[0];

    printid(a);
    for (i = 1; i < 5; i++)
    {
        if (x < a[i])
        {
            x = a[i];
            j = i;
        }
    }
    assert(x == 1488, "x must be 1488");
    assert(j == 2, "j must be 2");
}