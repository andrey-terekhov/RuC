int a = 9, *b, e = 1;

void main()
{
    float c = 3.14, *d;

    // Когда это сроки не было, тест компилировался, и не выдавало ни одной ошибки!!!
    b = &a;

    assert(*b == 9, "*b must be 9");

    (*b)++;

    assert(*b == 10, "*b must be 10");

    ++*b;
    ++c;
    -c--;
}