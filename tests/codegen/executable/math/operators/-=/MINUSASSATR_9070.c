float d[2] = {3.14, 2.72};

void MAIN()
{
    float r;
    r=d[1] -= 0.70;

    assert(d[0] == 3.140000, "d[0] must be 3.140000");
    assert(r == 2.020000, "r must be 2.020000");
}