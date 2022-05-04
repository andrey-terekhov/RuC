float d = 72;

void MAIN()
{
    float r;	
    r=d -= 70;

    assert(d == 2.000000, "d must be 2.000000");
    assert(r == 2.000000, "r must be 2.000000");
}