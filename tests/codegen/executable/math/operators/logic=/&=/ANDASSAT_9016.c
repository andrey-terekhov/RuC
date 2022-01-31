int d[2] =  {72, 1};
void MAIN()
{
    float r;
    r = d[1] &= 70;

    assert(d[0] == 72, "d[0] must be 72");
    assert(d[1] == 0, "d[1] must be 0");
}

