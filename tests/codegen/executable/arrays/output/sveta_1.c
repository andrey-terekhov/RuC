void main()
{
    char b[4] = "0123";
    b[1] = '?';

    assert(b[0] == '0', "b[0] must be 0");
    assert(b[1] == '?', "b[1] must be ?");
    assert(b[2] == '2', "b[2] must be 2");
    assert(b[3] == '3', "b[3] must be 3");
}
