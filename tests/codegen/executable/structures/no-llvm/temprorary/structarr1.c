struct{int a; float b;} c = {13, 3.14};
float d[2][] = {{1,2}, {1.1, 2.2, 3.3}};

void main()
{
    assert(c.a == 13, "c.a must be 13");
    assert(c.b == 3.14, "c.b must be 3.14");

    assert(d[0][0] == 1, "d[0][0] must be 1");
    assert(d[0][1] == 2, "d[0][1] must be 2");
    assert(d[1][0] == 1.1, "d[1][0] must be 1.1");
    assert(d[1][1] == 2.2, "d[1][1] must be 2.2");
    assert(d[1][2] == 3.3, "d[1][2] must be 3.3");
}
