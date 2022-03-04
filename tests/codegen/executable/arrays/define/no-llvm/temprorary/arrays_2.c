void main()
{
    char c[2][] = {"abc", "defg"};

    assert(c[0][0] == 'a', "c[0][0] must be a");
    assert(c[0][1] == 'b', "c[0][1] must be b");
    assert(c[0][2] == 'c', "c[0][2] must be c");
    assert(c[1][0] == 'd', "c[1][0] must be d");
    assert(c[1][1] == 'e', "c[1][1] must be e");
    assert(c[1][2] == 'f', "c[1][2] must be f");
    assert(c[1][3] == 'g', "c[1][3] must be g");
}
