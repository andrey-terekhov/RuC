int b[2] = {1, 2};

void main()
{
    ++b[1];

    assert(b[0] == 1, "b[0] must be 1");
    assert(b[1] == 3, "b[1] must be 3");
}
