int  b[3], c, d;

void main()
{
    d = b[2]++;
    c = d++;
    b[1] += 7; 

    assert(b[0] == 0, "b[0] must be 0");
    assert(b[1] == 7, "b[1] must be 7");
    assert(b[2] == 1, "b[2] must be 1");
    assert(c == 0, "c must be 0");
    assert(d == 1, "d must be 1");
}
