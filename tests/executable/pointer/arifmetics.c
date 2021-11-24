int a = 9, *b, e = 1;

void main()
{
    float c = 3.14, *d;

    assert(*b == 9, "*b must be 9");

    (*b)++;

    assert(*b == 10, "*b must be 10");

    ++*b;
    ++c;
    -c--;
}