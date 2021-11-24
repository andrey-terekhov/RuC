void main()
{
    int a = 9, *b, e = 1;
    float c = 3.14, *d;
    b = &a;

    assert(*b == 9, "*b must be 9");

    (*b)++;

    assert(*b == 10, "*b must be 10");

    ++*b;

    assert(*b == 11, "*b must be 1");
    
    d = &c;

    assert(*d == 3.14, "*d must be 3.14");
    (*d)++;

    assert(*d == 4.14, "*d must be 4.14");
    ++*d;

    assert(*d == 5.14, "*d must be 5.14");

    a = ++e + ++e;

    assert(a == 5, "a must be 5");
    assert(e == 3, "e must be 3");
    
    a = e++ + ++e;

    assert(a == 8, "a must be 8");
    assert(e == 5, "e must be 5");
}


