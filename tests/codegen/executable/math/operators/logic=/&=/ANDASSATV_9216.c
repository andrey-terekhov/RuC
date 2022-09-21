int d[2] =  {72, 1};
void MAIN()
{
    d[0] &= 64;

    assert(d[0] == 64, "d[0] must be 64");
    assert(d[1] == 1, "d[1] must be 1");
}
