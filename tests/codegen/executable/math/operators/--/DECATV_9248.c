int  b[2] = {4, 5};

void main()
{
    --b[1];

    assert(b[0] == 4, "b[0] must be 4");
    assert(b[1] == 4, "b[1] must be 4");
}
