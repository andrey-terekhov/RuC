struct{int a[2]; float b[3];} s;

void main()
{
    float r;
    s.a[0] = 13;
    r = s.b[0] = s.a[0];

    assert(r == 13, "r must be 13");
}
