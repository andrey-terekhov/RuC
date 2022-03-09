int j = -2, i = j, a = 2 * i - j;

void f();


void main()
{
    for(i = 0; i <= 10; i += 2)
    {
        j += i;
    };

    assert(j == 28, "j must be 28");

    a <<= 2;

    while(a > j)
    {
        f();
        a++;
    }

    assert(a == -8, "a must be -8");
}

void f()
{
    for ((i = 0, j = 9); i < 10 && j >- i; (i++, j--))
    {
        a += i + j;
    }
}
