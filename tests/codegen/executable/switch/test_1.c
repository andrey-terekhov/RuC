int a,   b;

void main()
{
    switch (a)
    {
        case 1:
            a = 1;
            break;
        case 2:
            a = 2;
            return;
        case 3:
            a = 3;
    }
    a = b;

    assert(a == 0, "a must be 0");
}