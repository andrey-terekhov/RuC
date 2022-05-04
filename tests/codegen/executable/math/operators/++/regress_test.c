void main()
{
    int a = 2, b = 3, c;
    int d[2] = {4, 2};
    float m[2] = {1.5, 2.5};
    float k;
   
    c = a * ++b;

    assert(c == 8, "c must be 8");

    c = a * b++;

    assert(c == 8, "c must be 8");
    assert(b == 5, "b must be 5");

    c = d[0]++;

    assert(c == 4, "c must be 4");
    
    c = d[0];

    assert(c == 5, "c must be 5");

    k = m[0]--;

    assert(k == 1.500000, "k must be 1.500000");
    
    k = m[0];

    assert(k == 0.500000, "k must be 0.500000");

    k = ++m[1];

    assert(k == 3.500000, "k must be 3.500000");

    c = --d[1];

    assert(c == 1, "c must be 1");
}