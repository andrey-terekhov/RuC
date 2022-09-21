int  b[3], c, d;

void main()
{
    b[1]++; 
    d++;
    d = b[1]++; 
    c = d++;

    assert(b[0] == 0, "b[0] must be 0");
    assert(b[1] == 2, "b[1] must be 2");
    assert(b[2] == 0, "b[2] must be 0");
    assert(c == 1, "c must be 1");
    assert(d == 2, "d must be 2");
}