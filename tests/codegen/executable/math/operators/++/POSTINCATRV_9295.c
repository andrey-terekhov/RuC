float  b[3], c, d;

void main()
{
    b[1]++; 
    d++;
    d = b[1]++; 
    c = d++;

    assert(b[0] == 0.000000, "b[0] must be 0.000000");
    assert(b[1] == 2.000000, "b[1] must be 2.000000");
    assert(b[2] == 0.000000, "b[2] must be 0.000000");
    assert(c == 1.000000, "c must be 1.000000");
    assert(d == 2.000000, "d must be 2.000000");
}
