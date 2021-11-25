float a, b[3] = {1.2e-1, 12e-2, 0.0012e2};
int i = 2;

void main()
{
    a = 12e-2;

    assert(a == 0, "a must be 0");
    assert(b[0] == 1.2e-1, "b[0] must be 1.2e-1");
    assert(b[1] == 12e-2, "b[1] must be 12e-2");
    assert(b[2] == 0.0012e2, "b[2] must be 0.0012e2");

    b[i] -= b[1] += i;

    assert(b[0] == 1.2e-1, "b[0] must be 1.2e-1");
    assert(b[1] == 12e-2 + i, "b[1] must be 12e-2 + 2");
    assert(b[2] == 0.0012e2 - (12e-2 + i), "b[2] must be 0.0012e2 - (12e-2 + i)");
}