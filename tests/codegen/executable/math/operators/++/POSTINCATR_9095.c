float  b[3], c, d;

void main()
{
    d = b[2]++;
    c = d++;
    b[1] += 7; 

    assert(b[0] == 0.000000, "b[0] must be 0.000000");
    assert(b[1] == 7.000000, "b[1] must be 7.000000");
    assert(b[2] == 1.000000, "b[2] must be 1.000000");
    assert(c == 0.000000, "c must be 0.000000");
    assert(d == 1.000000, "d must be 1.000000");
}
