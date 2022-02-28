float d[2] = {3.14, 2.72};

void MAIN()
{
    float r;
    r=d[1] -= 0.70;

    assert(abs(d[0] - 3.140000) < 0.000001, "d[0] must be 3.140000");
    assert(abs(r - 2.020000) < 0.000001, "r must be 2.020000");
}