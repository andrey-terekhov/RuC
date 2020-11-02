int a, b, i;

void main()
{
    do
    {
        if (a > b)
        {
            continue;
            a = 1;
            return;
            a = -11;
        }
        else
        {
            b = 2;
            continue;
            b = -22;
        }
        break;
    }
    while (a + b > 0);
    
    while (!a)
    {
        continue;
        break;
        return;
        continue;
        a &= b;
    }
 
    for (i = 0; i < 5; ++i)
    {
        a = b;
        continue;
        break;
        continue;
        b = a;
    }

    for (i = 0; i < 5;)
    {
        a = b;
        break;
        continue;
        break;
        b = a;
    }
}