float a, b[3] = {1.2e-1, 12e-2, 0.0012e2};
int i = 2;

void main()
{
    a = 12e-2;

    assert(abs(a - 12e-2) < 0.000001, "a must be 0");
    assert(abs(b[0] - 1.2e-1) < 0.000001, "b[0] must be 1.2e-1");
    assert(abs(b[1] - 12e-2) < 0.000001, "b[1] must be 12e-2");
    assert(abs(b[2] - 0.0012e2) < 0.000001, "b[2] must be 0.0012e2");

    b[i] -= b[1] += i;

    assert(abs(b[0] - 1.2e-1) < 0.000001, "b[0] must be 1.2e-1");
    assert(abs(b[1] - (12e-2 + i)) < 0.000001, "b[1] must be 12e-2 + 2");
    assert(abs(b[2] - (0.0012e2 - (12e-2 + i))) < 0.000001, "b[2] must be 0.0012e2 - (12e-2 + i)");
}