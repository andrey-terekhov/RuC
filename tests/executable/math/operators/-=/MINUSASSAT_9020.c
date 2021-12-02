int d[2] = {3, 72};

void MAIN()
{
    int r;	
    r = d[1] -= 70;

    assert(d[0] == 3, "d[0] must be 3");
    assert(r == 2, "r must be 2");
}
